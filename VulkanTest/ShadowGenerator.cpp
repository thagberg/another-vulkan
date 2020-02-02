#include "ShadowGenerator.h"

#include "descriptor-util.h"
#include "pipeline-util.h"

namespace hvk
{
	ShadowGenerator::ShadowGenerator(VkRenderPass renderPass, VkCommandPool commandPool) :
		DrawlistGenerator(renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo()
	{
		const VkDevice& device = GpuManager::getDevice();
		const VmaAllocator& allocator = GpuManager::getAllocator();

		// create descriptor set layout and descriptor pool
		auto poolSizes = util::descriptor::createPoolSizes<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(MAX_UBOS);
		util::descriptor::createDescriptorPool(device, poolSizes, MAX_DESCRIPTORS, mDescriptorPool);

		std::vector<VkDescriptorSetLayoutBinding> bindings = {
			util::descriptor::generateUboLayoutBinding(0, 1)
		};
		util::descriptor::createDescriptorSetLayout(device, bindings, mDescriptorSetLayout);

		// prepare pipeline
		preparePipelineInfo();

		// initialized
		setInitialized(true);
	}

	ShadowGenerator::~ShadowGenerator()
	{
		const VkDevice& device = GpuManager::getDevice();
		const VmaAllocator& allocator = GpuManager::getAllocator();

		vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);

		vkDestroyPipeline(device, mPipeline, nullptr);
		vkDestroyPipelineLayout(device, mPipelineInfo.pipelineLayout, nullptr);
	}

	void ShadowGenerator::preparePipelineInfo()
	{
		const auto& device = GpuManager::getDevice();

		VkPipelineLayoutCreateInfo layoutCreate = {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// sType
			nullptr,										// pNext
			0,												// flags
			1,												// setLayoutCount
			&mDescriptorSetLayout,							// pSetLayouts
			0,												// pushConstantRangeCount
			nullptr											// pPushConstantRanges
		};
		assert(vkCreatePipelineLayout(device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		// TODO: only need vertex position attribute; should we define a new vertex type for shadows?
		util::pipeline::fillVertexInfo<Vertex>(mPipelineInfo.vertexInfo);

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = 0;

		mPipelineInfo.blendAttachments = { blendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = "shaders/compiled/shadow_vert.spv";
		mPipelineInfo.fragShaderFile = "shaders/compiled/shadow_frag.spv";
		mPipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		mPipelineInfo.depthStencilState = util::pipeline::createDepthStencilState();

		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
	}

	void ShadowGenerator::invalidate()
	{
		setInitialized(false);
		vkDestroyPipeline(GpuManager::getDevice(), mPipeline, nullptr);
	}

	void ShadowGenerator::updateRenderPass(VkRenderPass renderPass)
	{
		mColorRenderPass = renderPass;
		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
		setInitialized(true);
	}

	ShadowBinding ShadowGenerator::createBinding()
	{
		ShadowBinding newBinding;

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