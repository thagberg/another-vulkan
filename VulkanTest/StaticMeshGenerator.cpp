#include "pch.h"
#include "StaticMeshGenerator.h"

#include "descriptor-util.h"
#include "pipeline-util.h"
#include "image-util.h"

namespace hvk
{
	StaticMeshGenerator::StaticMeshGenerator(
		VkRenderPass renderPass,
		VkCommandPool commandPool,
        HVK_shared<TextureMap> environmentMap,
		HVK_shared<TextureMap> irradianceMap,
		HVK_shared<TextureMap> brdfLutMap) :

		DrawlistGenerator(renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mLightsDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mLightsDescriptorSet(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo(),
		mLightsUbo(),
        mEnvironmentMap(environmentMap),
		mIrradianceMap(irradianceMap),
		mBrdfLutMap(brdfLutMap)
	{
        const VkDevice& device = GpuManager::getDevice();
        const VmaAllocator& allocator = GpuManager::getAllocator();

		/***************
		 Create descriptor set layout and descriptor pool
		***************/
		VkDescriptorSetLayoutBinding uboLayoutBinding = util::descriptor::generateUboLayoutBinding(0, 1);
		VkDescriptorSetLayoutBinding diffuseSamplerBinding = util::descriptor::generateSamplerLayoutBinding(1, 1);
		VkDescriptorSetLayoutBinding metalRoughSamplerBinding = util::descriptor::generateSamplerLayoutBinding(2, 1);
		VkDescriptorSetLayoutBinding normalSamplerBinding = util::descriptor::generateSamplerLayoutBinding(3, 1);
        VkDescriptorSetLayoutBinding environmentSamplerBinding = util::descriptor::generateSamplerLayoutBinding(4, 1);
		VkDescriptorSetLayoutBinding irradianceSamplerBinding = util::descriptor::generateSamplerLayoutBinding(5, 1);
		VkDescriptorSetLayoutBinding brdfSamplerBinding = util::descriptor::generateSamplerLayoutBinding(6, 1);

		std::vector<VkDescriptorSetLayoutBinding> bindings = {
            uboLayoutBinding, 
            diffuseSamplerBinding,
            metalRoughSamplerBinding,
            normalSamplerBinding,
            environmentSamplerBinding,
			irradianceSamplerBinding,
			brdfSamplerBinding
        };
		util::descriptor::createDescriptorSetLayout(device, bindings, mDescriptorSetLayout);

        auto poolSizes = util::descriptor::createPoolSizes<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(MAX_UBOS, MAX_SAMPLERS);
		util::descriptor::createDescriptorPool(device, poolSizes, MAX_DESCRIPTORS, mDescriptorPool);

		/*************
		 Create Lights UBO
		 *************/
		uint32_t uboMemorySize = sizeof(hvk::UniformLightObject<NUM_INITIAL_LIGHTS>);
        VkBufferCreateInfo uboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        uboInfo.size = uboMemorySize;
        uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo uniformAllocCreateInfo = {};
		uniformAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		uniformAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
            allocator,
			&uboInfo,
			&uniformAllocCreateInfo,
			&mLightsUbo.memoryResource,
			&mLightsUbo.allocation,
			&mLightsUbo.allocationInfo);

		/*****************
		 Create Lights descriptor set
		******************/
		VkDescriptorSetLayoutBinding lightLayoutBinding = util::descriptor::generateUboLayoutBinding(0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
		std::vector<decltype(lightLayoutBinding)> lightBindings = {
			lightLayoutBinding
		};
		util::descriptor::createDescriptorSetLayout(device, lightBindings, mLightsDescriptorSetLayout);

		std::vector<VkDescriptorSetLayout> lightLayouts = { mLightsDescriptorSetLayout };
		util::descriptor::allocateDescriptorSets(device, mDescriptorPool, mLightsDescriptorSet, lightLayouts);

		VkDescriptorBufferInfo lightsBufferInfo = {};
		lightsBufferInfo.buffer = mLightsUbo.memoryResource;
		lightsBufferInfo.offset = 0;
		lightsBufferInfo.range = NUM_INITIAL_LIGHTS * sizeof(hvk::UniformLight);

		VkWriteDescriptorSet lightsDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		lightsDescriptorWrite.dstSet = mLightsDescriptorSet;
		lightsDescriptorWrite.dstBinding = 0;
		lightsDescriptorWrite.dstArrayElement = 0;
		lightsDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightsDescriptorWrite.descriptorCount = 1;
		lightsDescriptorWrite.pBufferInfo = &lightsBufferInfo;

		vkUpdateDescriptorSets(device, 1, &lightsDescriptorWrite, 0, nullptr);

		/*
		 prepare graphics pipeline info	
		*/
		preparePipelineInfo();

		// initialized
		setInitialized(true);
	}

    StaticMeshGenerator::~StaticMeshGenerator()
    {
        const auto& device = GpuManager::getDevice();
        const auto& allocator = GpuManager::getAllocator();

		// TODO: need to make sure environment map and irradiance map are being cleaned up

        vmaDestroyBuffer(allocator, mLightsUbo.memoryResource, mLightsUbo.allocation);
        vkDestroyDescriptorSetLayout(device, mLightsDescriptorSetLayout, nullptr);

        vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);

        vkDestroyPipeline(device, mPipeline, nullptr);
        vkDestroyPipelineLayout(device, mPipelineInfo.pipelineLayout, nullptr);
    }

	void StaticMeshGenerator::preparePipelineInfo()
	{
        const auto& device = GpuManager::getDevice();

		VkPushConstantRange pushRange = {};
		pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushRange.size = sizeof(hvk::PushConstant);
		pushRange.offset = 0;

		std::array<VkDescriptorSetLayout, 2> dsLayouts = { mLightsDescriptorSetLayout, mDescriptorSetLayout };
		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = static_cast<uint32_t>(dsLayouts.size());
		layoutCreate.pSetLayouts = dsLayouts.data();
		layoutCreate.pushConstantRangeCount = 1;
		layoutCreate.pPushConstantRanges = &pushRange;

		assert(vkCreatePipelineLayout(device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		util::pipeline::fillVertexInfo<Vertex>(mPipelineInfo.vertexInfo);

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		mPipelineInfo.blendAttachments = { blendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = "shaders/compiled/vert.spv";
		mPipelineInfo.fragShaderFile = "shaders/compiled/frag.spv";
		mPipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		mPipelineInfo.depthStencilState = util::pipeline::createDepthStencilState();

		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
	}

	void StaticMeshGenerator::updateRenderPass(VkRenderPass renderPass)
	{
		mColorRenderPass = renderPass;
		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
		setInitialized(true);
	}

	void StaticMeshGenerator::invalidate()
	{
		setInitialized(false);
		vkDestroyPipeline(GpuManager::getDevice(), mPipeline, nullptr);
	}

	PBRBinding StaticMeshGenerator::createPBRBinding(const PBRMaterial& material)
	{
		PBRBinding newBinding;

        const auto& device = GpuManager::getDevice();
        const auto& allocator = GpuManager::getAllocator();
		//const auto& comp = registry.get<PBRMaterial>(pbrEntity);

		// Create UBO
        uint32_t uboMemorySize = sizeof(hvk::UniformBufferObject);
        VkBufferCreateInfo uboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        uboInfo.size = uboMemorySize;
        uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo uniformAllocCreateInfo = {};
		uniformAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		uniformAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
            allocator,
			&uboInfo,
			&uniformAllocCreateInfo,
			&newBinding.ubo.memoryResource,
			&newBinding.ubo.allocation,
			nullptr);

		VkDescriptorSetAllocateInfo dsAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		dsAlloc.descriptorPool = mDescriptorPool;
		dsAlloc.descriptorSetCount = 1;
		dsAlloc.pSetLayouts = &mDescriptorSetLayout;

		assert(vkAllocateDescriptorSets(device, &dsAlloc, &newBinding.descriptorSet) == VK_SUCCESS);

		// Update descriptor set
		{
			std::vector<VkWriteDescriptorSet> descriptorWrites;
			descriptorWrites.reserve(7);

			std::vector<VkDescriptorBufferInfo> bufferInfos = {
				VkDescriptorBufferInfo {
					newBinding.ubo.memoryResource,
					0,
					sizeof(hvk::UniformBufferObject) } };

			auto bufferDescriptorWrite = util::descriptor::createDescriptorBufferWrite(bufferInfos, newBinding.descriptorSet, 0);
			descriptorWrites.push_back(bufferDescriptorWrite);

			std::vector<VkDescriptorImageInfo> albedoImageInfos = {
				VkDescriptorImageInfo{
					material.albedo.sampler,
					material.albedo.view,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };

			auto albedoDescriptorWrite = util::descriptor::createDescriptorImageWrite(
				albedoImageInfos,
				newBinding.descriptorSet,
				1);
			descriptorWrites.push_back(albedoDescriptorWrite);

			std::vector<VkDescriptorImageInfo> mtlRoughImageInfos = {
				VkDescriptorImageInfo{
				material.metallicRoughness.sampler,
				material.metallicRoughness.view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
			auto mtlRoughDescriptorWrite = util::descriptor::createDescriptorImageWrite(mtlRoughImageInfos, newBinding.descriptorSet, 2);
			descriptorWrites.push_back(mtlRoughDescriptorWrite);

			std::vector<VkDescriptorImageInfo> normalImageInfos = {
				VkDescriptorImageInfo{
				material.normal.sampler,
				material.normal.view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
			auto normalDescriptorWrite = util::descriptor::createDescriptorImageWrite(normalImageInfos, newBinding.descriptorSet, 3);
			descriptorWrites.push_back(normalDescriptorWrite);

            std::vector<VkDescriptorImageInfo> environmentImageInfos = {
                VkDescriptorImageInfo{
                    mEnvironmentMap->sampler,
                    mEnvironmentMap->view,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL} };
            auto environmentDescriptorWrite = util::descriptor::createDescriptorImageWrite(
                environmentImageInfos, 
                newBinding.descriptorSet, 
                4);
			descriptorWrites.push_back(environmentDescriptorWrite);

			std::vector<VkDescriptorImageInfo> irradianceImageInfos = {
				VkDescriptorImageInfo{
					mIrradianceMap->sampler,
					mIrradianceMap->view,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }};
			auto irradianceDescriptorWrite = util::descriptor::createDescriptorImageWrite(
				irradianceImageInfos,
				newBinding.descriptorSet,
				5);
			descriptorWrites.push_back(irradianceDescriptorWrite);

			std::vector<VkDescriptorImageInfo> brdfImageInfos = {
				VkDescriptorImageInfo{
					mBrdfLutMap->sampler,
					mBrdfLutMap->view,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }};
			auto brdfDescriptorWrite = util::descriptor::createDescriptorImageWrite(
				brdfImageInfos,
				newBinding.descriptorSet,
				6);
			descriptorWrites.push_back(brdfDescriptorWrite);

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		return newBinding;
	}
}