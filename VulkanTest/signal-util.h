#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

namespace hvk
{
	namespace util
	{
		namespace signal
		{
			VkSemaphore createSemaphore(VkDevice device);

			VkFence createFence(VkDevice device, VkFenceCreateFlagBits flags = VK_FENCE_CREATE_SIGNALED_BIT);
		}
	}
}
