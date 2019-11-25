#pragma once

#include <vector>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

namespace hvk
{
	namespace util
	{
		namespace renderpass
		{
			VkAttachmentDescription createColorAttachment(
				VkFormat imageFormat,
				VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			VkAttachmentDescription createDepthAttachment();

			VkRenderPass createRenderPass(
				VkDevice device, 
				VkFormat swapchainImageFormat, 
				std::vector<VkSubpassDependency>& subpassDependencies,
				const VkAttachmentDescription* pColorAttachment, 
				const VkAttachmentDescription* pDepthAttachment=nullptr);
		}
	}
}
