#include "pch.h"
#include "QuadGenerator.h"

#include "descriptor-util.h"
#include "pipeline-util.h"

namespace hvk
{
	QuadGenerator::QuadGenerator(
		VulkanDevice device,
		VmaAllocator allocator,
		VkQueue graphicsQueue,
		VkRenderPass renderPass,
		VkCommandPool commandPool,
		HVK_shared<TextureMap> offscreenMap,
		std::array<std::string, 2>& shaderFiles) :

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

		std::vector<VkDescriptorSetLayout> layouts;
		if (mOffscreenMap != nullptr)
		{
			// Create descriptor pool
			auto poolSizes = util::descriptor::createPoolSizes<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(1);
			util::descriptor::createDescriptorPool(mDevice.device, poolSizes, 1, mDescriptorPool);

			// Create descriptor set layout
			auto quadSamplerBinding = util::descriptor::generateSamplerLayoutBinding(0, 1);
			std::vector<decltype(quadSamplerBinding)> bindings = { quadSamplerBinding };
			util::descriptor::createDescriptorSetLayout(mDevice.device, bindings, mDescriptorSetLayout);

			// Create descriptor set
			layouts.push_back(mDescriptorSetLayout);
			util::descriptor::allocateDescriptorSets(mDevice.device, mDescriptorPool, mDescriptorSet, layouts);

			// Update descriptor set
			VkDescriptorImageInfo imageInfo = {
				mOffscreenMap->sampler,
				mOffscreenMap->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
			std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
			auto imageWrite = util::descriptor::createDescriptorImageWrite(imageInfos, mDescriptorSet, 0);

			std::vector<VkWriteDescriptorSet> descriptorWrites = { imageWrite };
			util::descriptor::writeDescriptorSets(mDevice.device, descriptorWrites);
		}

		// Prepare pipeline
		VkPushConstantRange pushRange = {};
		pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushRange.size = sizeof(ExposureSettings);
		pushRange.offset = 0;

		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = static_cast<uint32_t>(layouts.size());
		layoutCreate.pSetLayouts = layouts.data();
		layoutCreate.pushConstantRangeCount = 1;
		layoutCreate.pPushConstantRanges = &pushRange;
		assert(vkCreatePipelineLayout(mDevice.device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		util::pipeline::fillVertexInfo<QuadVertex>(mPipelineInfo.vertexInfo);

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		mPipelineInfo.blendAttachments = { blendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = shaderFiles[0];
		mPipelineInfo.fragShaderFile = shaderFiles[1];
		mPipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		mPipelineInfo.depthStencilState = util::pipeline::createDepthStencilState(false, false);

		mPipeline = generatePipeline(mDevice, mColorRenderPass, mPipelineInfo);

		setInitialized(true);
	}

	QuadGenerator::~QuadGenerator()
	{
        vmaDestroyBuffer(mAllocator, mRenderable.vbo.memoryResource, mRenderable.vbo.allocation);
        vmaDestroyBuffer(mAllocator, mRenderable.ibo.memoryResource, mRenderable.ibo.allocation);
	}

	void QuadGenerator::invalidate()
	{
		setInitialized(false);
        vkDestroyPipeline(mDevice.device, mPipeline, nullptr);
	}

	void QuadGenerator::updateRenderPass(VkRenderPass renderPass)
	{
        mColorRenderPass = renderPass;

		if (mOffscreenMap != nullptr)
		{
			// need to re-apply the color attachment since it was destroyed
			// when the resolution changed
			VkDescriptorImageInfo imageInfo = {
				mOffscreenMap->sampler,
				mOffscreenMap->view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
			std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
			auto imageWrite = util::descriptor::createDescriptorImageWrite(imageInfos, mDescriptorSet, 0);

			std::vector<VkWriteDescriptorSet> descriptorWrites = { imageWrite };
			util::descriptor::writeDescriptorSets(mDevice.device, descriptorWrites);
		}
        mPipeline = generatePipeline(mDevice, mColorRenderPass, mPipelineInfo);
		setInitialized(true);
	}


    VkCommandBuffer& QuadGenerator::drawFrame(
        const VkCommandBufferInheritanceInfo& inheritance,
        const VkFramebuffer& framebuffer,
        const VkViewport& viewport,
        const VkRect2D& scissor,
		const ExposureSettings& exposure)
    {
        // record commands
        VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        commandBegin.pInheritanceInfo = &inheritance;

        assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);

        // bind pipeline
        vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
        vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mRenderable.vbo.memoryResource, offsets);
        vkCmdBindIndexBuffer(mCommandBuffer, mRenderable.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
		if (mOffscreenMap != nullptr)
		{
			vkCmdBindDescriptorSets(
				mCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				mPipelineInfo.pipelineLayout,
				0,
				1,
				&mDescriptorSet,
				0,
				nullptr);
		}

		vkCmdPushConstants(
			mCommandBuffer, 
			mPipelineInfo.pipelineLayout, 
			VK_SHADER_STAGE_FRAGMENT_BIT, 
			0, 
			sizeof(ExposureSettings), 
			&exposure);

        vkCmdDrawIndexed(mCommandBuffer, numIndices, 1, 0, 0, 0);
        assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

        return mCommandBuffer;
    }
}
