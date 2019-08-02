#include "stdafx.h"
#include <assert.h>

#include "Renderer.h"

namespace hvk {

	Renderer::Renderer()
	{
	}


	Renderer::~Renderer()
	{
	}

	void Renderer::init(VulkanDevice device, CameraRef camera) {
		mDevice = device;
		mCamera = camera;

		// Create fence for rendering complete
		VkFenceCreateInfo fenceCreate = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreate.pNext = nullptr;
		fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		assert(vkCreateFence(mDevice.device, &fenceCreate, nullptr, &mRenderFence) == VK_SUCCESS);

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

		mInitialized = true;
	}
}
