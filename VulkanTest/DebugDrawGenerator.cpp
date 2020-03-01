#include "pch.h"
#include "DebugDrawGenerator.h"

#include "descriptor-util.h"
#include "pipeline-util.h"
#include "GpuManager.h"

namespace hvk
{

	DebugDrawGenerator::DebugDrawGenerator(
			VkRenderPass renderPass,
			VkCommandPool commandPool) :

		DrawlistGenerator(renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo()
	{
        const VkDevice& device = GpuManager::getDevice();

		/***************
		 Create descriptor set layout and descriptor pool
		***************/
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		assert(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mDescriptorSetLayout) == VK_SUCCESS);

		auto poolSizes = util::descriptor::createPoolSizes<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(MAX_UBOS);
		util::descriptor::createDescriptorPool(device, poolSizes, MAX_DESCRIPTORS, mDescriptorPool);

		/**************
		Set up pipeline
		**************/
		std::array<VkDescriptorSetLayout, 1> dsLayouts = { mDescriptorSetLayout };
		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = static_cast<uint32_t>(dsLayouts.size());
		layoutCreate.pSetLayouts = dsLayouts.data();
		layoutCreate.pushConstantRangeCount = 0;

		assert(vkCreatePipelineLayout(device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		util::pipeline::fillVertexInfo<ColorVertex>(mPipelineInfo.vertexInfo);

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		mPipelineInfo.blendAttachments = { blendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = "shaders/compiled/normal_v.spv";
		mPipelineInfo.fragShaderFile = "shaders/compiled/normal_f.spv";
		mPipelineInfo.depthStencilState = util::pipeline::createDepthStencilState();
		mPipelineInfo.rasterizationState = util::pipeline::createRasterizationState(VK_POLYGON_MODE_LINE);

		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);

		setInitialized(true);
	}


	DebugDrawGenerator::~DebugDrawGenerator()
	{
        const auto& device = GpuManager::getDevice();
        const auto& allocator = GpuManager::getAllocator();

        vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);

        vkDestroyPipeline(device, mPipeline, nullptr);
        vkDestroyPipelineLayout(device, mPipelineInfo.pipelineLayout, nullptr);
	}

	void DebugDrawGenerator::invalidate()
	{
		setInitialized(false);
		vkDestroyPipeline(GpuManager::getDevice(), mPipeline, nullptr);
	}

	void DebugDrawGenerator::updateRenderPass(VkRenderPass renderPass)
	{
		mColorRenderPass = renderPass;
		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
		setInitialized(true);
	}

	DebugDrawBinding DebugDrawGenerator::createDebugDrawBinding()
	{
		DebugDrawBinding newBinding;

		const auto& device = GpuManager::getDevice();
		const auto& allocator = GpuManager::getAllocator();

		// create UBO
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

		// create descriptor set
		VkDescriptorSetAllocateInfo dsAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		dsAlloc.descriptorPool = mDescriptorPool;
		dsAlloc.descriptorSetCount = 1;
		dsAlloc.pSetLayouts = &mDescriptorSetLayout;

		assert(vkAllocateDescriptorSets(device, &dsAlloc, &newBinding.descriptorSet) == VK_SUCCESS);

		// update descriptor set
		std::vector<VkWriteDescriptorSet> descriptorWrites;
		std::vector<VkDescriptorBufferInfo> bufferInfos = {
			VkDescriptorBufferInfo {
				newBinding.ubo.memoryResource,
				0,
				sizeof(hvk::UniformBufferObject) } };
		auto bufferDescriptorWrite = util::descriptor::createDescriptorBufferWrite(bufferInfos, newBinding.descriptorSet, 0);
		descriptorWrites.push_back(bufferDescriptorWrite);
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		return newBinding;
	}
}
