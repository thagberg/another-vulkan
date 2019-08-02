#include "stdafx.h"
#include <assert.h>
#include <array>
#include <vector>

/*
*/
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer.h"
#define HVK_UTIL_IMPLEMENTATION
#include "vulkan-util.h"

#define MAX_DESCRIPTORS 20
#define NUM_INITIAL_RENDEROBJECTS 10

namespace hvk {

	Renderer::Renderer() :
		mFirstRenderIndexAvailable(-1)
	{
	}


	Renderer::~Renderer()
	{
	}

	void Renderer::init(
		VulkanDevice device, 
		VmaAllocator allocator,
		VkQueue graphicsQueue,
		VkRenderPass renderPass,
		CameraRef camera, 
		VkFormat colorAttachmentFormat,
		VkExtent2D extent) {

		mDevice = device;
		mGraphicsQueue = graphicsQueue;
		mRenderPass = renderPass;
		mExtent = extent;
		mCamera = camera;
		mAllocator = allocator;

		mRenderables.reserve(NUM_INITIAL_RENDEROBJECTS);

		// Create fence for rendering complete
		VkFenceCreateInfo fenceCreate = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreate.pNext = nullptr;
		fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		assert(vkCreateFence(mDevice.device, &fenceCreate, nullptr, &mRenderFence) == VK_SUCCESS);

		// create semaphore for rendering finished
		mRenderFinished = createSemaphore(mDevice.device);

		// Create command pool
		VkCommandPoolCreateInfo poolCreate = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreate.queueFamilyIndex = device.queueFamilies.graphics;
		poolCreate.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		assert(vkCreateCommandPool(mDevice.device, &poolCreate, nullptr, &mCommandPool) == VK_SUCCESS);

		// Allocate command buffer
		VkCommandBufferAllocateInfo bufferAlloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		bufferAlloc.commandBufferCount = 1;
		bufferAlloc.commandPool = mCommandPool;
		bufferAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		assert(vkAllocateCommandBuffers(mDevice.device, &bufferAlloc, &mCommandBuffer) == VK_SUCCESS);

		// Create descriptor set layout and descriptor pool
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerLayoutBiding = {};
		samplerLayoutBiding.binding = 1;
		samplerLayoutBiding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBiding.descriptorCount = 1;
		samplerLayoutBiding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBiding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBiding };
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		assert(vkCreateDescriptorSetLayout(mDevice.device, &layoutInfo, nullptr, &mDescriptorSetLayout) == VK_SUCCESS);

		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = MAX_DESCRIPTORS;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = MAX_DESCRIPTORS;

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = MAX_DESCRIPTORS;

		assert(vkCreateDescriptorPool(mDevice.device, &poolInfo, nullptr, &mDescriptorPool) == VK_SUCCESS);

		// Create graphics pipeline
		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = 1;
		layoutCreate.pSetLayouts = &mDescriptorSetLayout;
		layoutCreate.pushConstantRangeCount = 0;
		layoutCreate.pPushConstantRanges = nullptr;

		assert(vkCreatePipelineLayout(mDevice.device, &layoutCreate, nullptr, &mPipelineLayout) == VK_SUCCESS);

		mPipeline = createGraphicsPipeline(mDevice.device, extent, mRenderPass, mPipelineLayout);

		mInitialized = true;
	}

	void Renderer::addRenderable(RenderObjRef renderObject) {
		Renderable newRenderable;
		newRenderable.renderObject = renderObject;

		std::vector<Vertex> vertices = renderObject->getVertices();
		std::vector<uint16_t> indices = renderObject->getIndices();
		newRenderable.numVertices = vertices.size();
		newRenderable.numIndices = indices.size();

		// Create vertex buffer
        uint32_t vertexMemorySize = sizeof(vertices[0]) * vertices.size();
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
            &newRenderable.vbo.memoryResource,
            &newRenderable.vbo.allocation,
            &newRenderable.vbo.allocationInfo);

        memcpy(newRenderable.vbo.allocationInfo.pMappedData, vertices.data(), (size_t)vertexMemorySize);

		// Create index buffer
        uint32_t indexMemorySize = sizeof(uint16_t) * indices.size();
        VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        iboInfo.size = indexMemorySize;
        iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocCreateInfo = {};
        indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        indexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(
            mAllocator,
            &iboInfo,
            &indexAllocCreateInfo,
            &newRenderable.ibo.memoryResource,
            &newRenderable.ibo.allocation,
            &newRenderable.ibo.allocationInfo);

        memcpy(newRenderable.ibo.allocationInfo.pMappedData, indices.data(), (size_t)indexMemorySize);

        // create UBOs
        uint32_t uboMemorySize = sizeof(hvk::UniformBufferObject);
        VkBufferCreateInfo uboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        uboInfo.size = uboMemorySize;
        uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo uniformAllocCreateInfo = {};
		uniformAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		uniformAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
			mAllocator,
			&uboInfo,
			&uniformAllocCreateInfo,
			&newRenderable.ubo.memoryResource,
			&newRenderable.ubo.allocation,
			&newRenderable.ubo.allocationInfo);

		// create texture
        newRenderable.texture = createTextureImage(mDevice.device, mAllocator, mCommandPool, mGraphicsQueue);
        newRenderable.textureView = createImageView(mDevice.device, newRenderable.texture.memoryResource, VK_FORMAT_R8G8B8A8_UNORM);
        newRenderable.textureSampler = createTextureSampler(mDevice.device);

		// create descriptor set
		VkDescriptorSetAllocateInfo dsAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		dsAlloc.descriptorPool = mDescriptorPool;
		dsAlloc.descriptorSetCount = 1;
		dsAlloc.pSetLayouts = &mDescriptorSetLayout;

		assert(vkAllocateDescriptorSets(mDevice.device, &dsAlloc, &newRenderable.descriptorSet) == VK_SUCCESS);

		// Update descriptor set
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = newRenderable.ubo.memoryResource;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(hvk::UniformBufferObject);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = newRenderable.textureView;
			imageInfo.sampler = newRenderable.textureSampler;

			VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrite.dstSet = newRenderable.descriptorSet;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			VkWriteDescriptorSet imageDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			imageDescriptorWrite.dstSet = newRenderable.descriptorSet;
			imageDescriptorWrite.dstBinding = 1;
			imageDescriptorWrite.dstArrayElement = 0;
			imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageDescriptorWrite.descriptorCount = 1;
			imageDescriptorWrite.pImageInfo = &imageInfo;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = { descriptorWrite, imageDescriptorWrite };

			vkUpdateDescriptorSets(mDevice.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}

		//findFirstRenderIndexAvailable();
		//mRenderables[mFirstRenderIndexAvailable] = newRenderable;
		mRenderables.push_back(newRenderable);
	}

	void Renderer::findFirstRenderIndexAvailable() {
		// TODO: actually look for the next available index
		//		 and resize the vector if necessary
		mFirstRenderIndexAvailable++;
	}

	void Renderer::recordCommandBuffer(VkFramebuffer& framebuffer) {
		assert(vkWaitForFences(mDevice.device, 1, &mRenderFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);
		assert(vkResetFences(mDevice.device, 1, &mRenderFence) == VK_SUCCESS);

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		commandBegin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBegin.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderBegin.renderPass = mRenderPass;
		renderBegin.framebuffer = framebuffer;
		renderBegin.renderArea.offset = { 0, 0 };
		renderBegin.renderArea.extent = mExtent;
		renderBegin.clearValueCount = clearValues.size();
		renderBegin.pClearValues = clearValues.data();

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBeginRenderPass(mCommandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		VkDeviceSize offsets[] = { 0 };
		
		for (const auto& renderable : mRenderables) {
			vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &renderable.vbo.memoryResource, offsets);
			vkCmdBindIndexBuffer(mCommandBuffer, renderable.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(
				mCommandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mPipelineLayout, 
				0, 
				1, 
				&renderable.descriptorSet, 
				0, 
				nullptr);
			vkCmdDrawIndexed(mCommandBuffer, renderable.numIndices, 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(mCommandBuffer);
		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);
	}

	VkSemaphore Renderer::drawFrame(VkFramebuffer& framebuffer, VkSemaphore* waitSemaphores, uint32_t waitSemaphoreCount) {
		// update uniforms
		for (const auto& renderable : mRenderables) {
			UniformBufferObject ubo = {};
			ubo.model = renderable.renderObject->getWorldTransform();
			ubo.view = mCamera->getWorldTransform() * glm::lookAt(glm::vec3(0.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
			ubo.modelViewProj = mCamera->getProjection() * ubo.view * ubo.model;
			ubo.modelViewProj[1][1] *= -1;
			memcpy(renderable.ubo.allocationInfo.pMappedData, &ubo, sizeof(ubo));
		}

		// record the command buffer
		recordCommandBuffer(framebuffer);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.waitSemaphoreCount = waitSemaphoreCount;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mCommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &mRenderFinished;

		assert(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mRenderFence) == VK_SUCCESS);

		// caller can determine when render is finished for present
		return mRenderFinished;
	}
}
