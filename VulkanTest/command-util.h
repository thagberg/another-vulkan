#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

namespace hvk
{
	namespace util
	{
		namespace command
		{
			VkCommandBuffer beginSingleTimeCommand(
				VkDevice device, 
				VkCommandPool commandPool);

			void endSingleTimeCommand(
				VkDevice device, 
				VkCommandPool commandPool, 
				VkCommandBuffer commandBuffer, 
				VkQueue queue);

			VkCommandPool createCommandPool(
				VkDevice device,
				int queueFamilyIndex,
				VkCommandPoolCreateFlags flags);
		}
	}
}
