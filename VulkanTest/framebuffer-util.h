#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

namespace hvk
{
	namespace util
	{
		namespace framebuffer
		{
			void createFramebuffer(
				VkDevice device,
				const VkRenderPass& renderPass,
				const VkExtent2D& extent,
				const VkImageView* pImageView,
				const VkImageView* pDepthView,
				VkFramebuffer* oFramebuffer);
		}
	}
}
