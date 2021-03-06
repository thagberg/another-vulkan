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

			VkAttachmentDescription createDepthAttachment(
                VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                VkImageLayout finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			VkRenderPass createRenderPass(
				VkDevice device, 
				std::vector<VkSubpassDependency>& subpassDependencies,
				const VkAttachmentDescription* pColorAttachment, 
				const VkAttachmentDescription* pDepthAttachment=nullptr);

			VkRenderPass createRenderPass(
				VkDevice device,
				const std::vector<VkSubpassDescription>& subpassDescriptions,
				const std::vector<VkSubpassDependency>& subpassDependencies,
				const std::vector<VkAttachmentDescription>& attachmentDescriptions);

			VkSubpassDependency createSubpassDependency(
				uint32_t srcSubpass = VK_SUBPASS_EXTERNAL,
				uint32_t dstSubpass = 0,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VkAccessFlags srcAccessMask = 0,
				VkAccessFlags dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VkDependencyFlags dependencyFlags=0);

			VkSubpassDescription createSubpassDescription(
				const std::vector<VkAttachmentReference>& colorAttachments,
				const std::vector<VkAttachmentReference>& inputAttachments,
				const VkAttachmentReference* depthStencilAttachment = nullptr,
				VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
		}
	}
}
