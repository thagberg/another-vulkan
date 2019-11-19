#include "pch.h"
#include "QuadGenerator.h"
#include "vulkan-util.h"

namespace hvk
{
	QuadGenerator::QuadGenerator(
		VulkanDevice device,
		VmaAllocator allocator,
		VkQueue graphicsQueue,
		VkRenderPass renderPass,
		VkCommandPool commandPool,
		HVK_shared<TextureMap> offscreenMap) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mDescriptorSet(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo(),
		mRenderable(),
		mOffscreenMap(offscreenMap)
	{
		// Create VBO
		size_t vertexMemorySize = sizeof(QuadVertex) * numVertices;
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = vertexMemorySize;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
			mAllocator,
			&bufferInfo,
			&allocCreateInfo,
			&mRenderable.vbo.memoryResource,
			&mRenderable.vbo.allocation,
			&mRenderable.vbo.allocationInfo);
		memcpy(mRenderable.vbo.allocationInfo.pMappedData, quadVertices.data(), vertexMemorySize);

		// Create IBO
		size_t indexMemorySize = sizeof(uint16_t) * numIndices;
		VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		iboInfo.size = indexMemorySize;
		iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		VmaAllocationCreateInfo indexCreateInfo = {};
		indexCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		indexCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
			mAllocator,
			&iboInfo,
			&indexCreateInfo,
			&mRenderable.ibo.memoryResource,
			&mRenderable.ibo.allocation,
			&mRenderable.ibo.allocationInfo);
		memcpy(mRenderable.ibo.allocationInfo.pMappedData, quadIndices.data(), indexMemorySize);

		// Create descriptor pool
		auto poolSizes = createPoolSizes<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(1);
		createDescriptorPool(mDevice.device, poolSizes, 1, mDescriptorPool);

		// Create descriptor set layout
		auto quadSamplerBinding = generateSamplerLayoutBinding(1, 1);
		std::vector<decltype(quadSamplerBinding)> bindings = { quadSamplerBinding };
		createDescriptorSetLayout(mDevice.device, bindings, mDescriptorSetLayout);

		// Create descriptor set
		std::vector<VkDescriptorSetLayout> layouts = { mDescriptorSetLayout };
		allocateDescriptorSets(mDevice.device, mDescriptorPool, mDescriptorSet, layouts);

		// Update descriptor set
		VkDescriptorImageInfo imageInfo = {
			mOffscreenMap->sampler,
			mOffscreenMap->view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
		auto imageWrite = createDescriptorImageWrite(imageInfos, mDescriptorSet, 0);

		std::vector<VkWriteDescriptorSet> descriptorWrites = { imageWrite };
		writeDescriptorSets(mDevice.device, descriptorWrites);

		// Prepare pipeline
	}

	QuadGenerator::~QuadGenerator()
	{

	}

	void QuadGenerator::invalidate()
	{
		setInitialized(false);
	}

	void QuadGenerator::updateRenderPass(VkRenderPass renderPass)
	{
		setInitialized(true);
	}
}
