#include "pch.h"
#include "renderpass-util.h"

#include <assert.h>

namespace hvk
{
	namespace util
	{
		namespace renderpass
		{
			VkAttachmentDescription createColorAttachment(
				VkFormat imageFormat,
				VkImageLayout initialLayout,
				VkImageLayout finalLayout)
			{
				VkAttachmentDescription colorAttachment = {};
				colorAttachment.format = imageFormat;
				colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				colorAttachment.initialLayout = initialLayout;
				colorAttachment.finalLayout = finalLayout;

				return colorAttachment;
			}


			VkAttachmentDescription createDepthAttachment()
			{
				VkAttachmentDescription depthAttachment = {};
				depthAttachment.format = VK_FORMAT_D32_SFLOAT;  // TODO: make this dynamic
				depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

				return depthAttachment;
			}


			VkRenderPass createRenderPass(
				VkDevice device, 
				VkFormat swapchainImageFormat, 
				std::vector<VkSubpassDependency>& subpassDependencies,
				const VkAttachmentDescription* pColorAttachment, 
				const VkAttachmentDescription* pDepthAttachment)
			{
				VkRenderPass renderPass;

				VkSubpassDescription subpass = {};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.colorAttachmentCount = 1;

				std::vector<VkAttachmentDescription> attachments;
				if (pColorAttachment != nullptr && pDepthAttachment != nullptr) {
					attachments.reserve(2);
				}

				VkAttachmentReference colorAttachmentRef = {};
				if (pColorAttachment != nullptr) {
					colorAttachmentRef.attachment = 0;
					colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

					attachments.push_back(*pColorAttachment);
					subpass.pColorAttachments = &colorAttachmentRef;
				}
				else {
					subpass.pColorAttachments = nullptr;
				}

				VkAttachmentReference depthAttachmentRef = {};
				if (pDepthAttachment != nullptr) {
					depthAttachmentRef.attachment = 1;
					depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

					attachments.push_back(*pDepthAttachment);
					subpass.pDepthStencilAttachment = &depthAttachmentRef;
				}
				else {
					subpass.pDepthStencilAttachment = nullptr;
				}

				VkRenderPassCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
				createInfo.pAttachments = attachments.data();
				createInfo.subpassCount = 1;
				createInfo.pSubpasses = &subpass;
				createInfo.dependencyCount = subpassDependencies.size();
				createInfo.pDependencies = subpassDependencies.data();

				assert(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) == VK_SUCCESS);

				return renderPass;
			}
		}
	}
}