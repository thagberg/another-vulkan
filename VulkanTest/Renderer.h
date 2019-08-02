#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "types.h"
#include "Node.h"
#include "Camera.h"

namespace hvk {

	class Renderer
	{
	private:
		bool mInitialized;
		VkFence mRenderFence;
		VkCommandPool mCommandPool;
		VkCommandBuffer mCommandBuffer;
		VulkanDevice mDevice;
		CameraRef mCamera;

		void recordCommandBuffer();

	public:
		Renderer();
		~Renderer();

		void init(VulkanDevice device, CameraRef camera);
	};
}

