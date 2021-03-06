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


			VkAttachmentDescription createDepthAttachment(
                VkImageLayout initialLayout,
                VkImageLayout finalLayout)
			{
				VkAttachmentDescription depthAttachment = {};
				depthAttachment.format = VK_FORMAT_D32_SFLOAT;  // TODO: make this dynamic
				depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.initialLayout = initialLayout;
				depthAttachment.finalLayout = finalLayout;

				return depthAttachment;
			}


			VkRenderPass createRenderPass(
				VkDevice device, 
				std::vector<VkSubpassDependency>& subpassDependencies,
				const VkAttachmentDescription* pColorAttachment, 
				const VkAttachmentDescription* pDepthAttachment)
			{
				VkRenderPass renderPass;

				VkSubpassDescription subpass = {};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.colorAttachmentCount = (pColorAttachment) ? 1 : 0;

				std::vector<VkAttachmentDescription> attachments;
                size_t numAttachments = 0;
                if (pColorAttachment)
                {
                    numAttachments++;
                }
                if (pDepthAttachment)
                {
                    numAttachments++;
                }
                attachments.reserve(numAttachments);

                uint32_t currentAttachment = 0;
				VkAttachmentReference colorAttachmentRef = {};
				if (pColorAttachment != nullptr) {
					colorAttachmentRef.attachment = currentAttachment++;
					colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

					attachments.push_back(*pColorAttachment);
					subpass.pColorAttachments = &colorAttachmentRef;
				}
				else {
					subpass.pColorAttachments = nullptr;
				}

				VkAttachmentReference depthAttachmentRef = {};
				if (pDepthAttachment != nullptr) {
					depthAttachmentRef.attachment = currentAttachment++;
					depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

					attachments.push_back(*pDepthAttachment);
					subpass.pDepthStencilAttachment = &depthAttachmentRef;
				}
				else {
					subpass.pDepthStencilAttachment = nullptr;
				}

				std::vector<VkSubpassDescription> subpasses = {
					subpass
				};
				return createRenderPass(device, subpasses, subpassDependencies, attachments);

			}

			VkRenderPass createRenderPass(
				VkDevice device,
				const std::vector<VkSubpassDescription>& subpassDescriptions,
				const std::vector<VkSubpassDependency>& subpassDependencies,
				const std::vector<VkAttachmentDescription>& attachmentDescriptions)
			{
				VkRenderPass renderpass;

				VkRenderPassCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				createInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
				createInfo.pAttachments = attachmentDescriptions.data();
				createInfo.subpassCount = subpassDescriptions.size();
				createInfo.pSubpasses = subpassDescriptions.data();
				createInfo.dependencyCount = subpassDependencies.size();
				createInfo.pDependencies = subpassDependencies.data();

				assert(vkCreateRenderPass(device, &createInfo, nullptr, &renderpass) == VK_SUCCESS);

				return renderpass;
			}


			VkSubpassDependency createSubpassDependency(
				uint32_t srcSubpass,
				uint32_t dstSubpass,
				VkPipelineStageFlags srcStageMask,
				VkPipelineStageFlags dstStageMask,
				VkAccessFlags srcAccessMask,
				VkAccessFlags dstAccessMask,
				VkDependencyFlags dependencyFlags)
			{
				VkSubpassDependency dependency = {};
				dependency.srcSubpass = srcSubpass;
				dependency.dstSubpass = dstSubpass;
				dependency.srcStageMask = srcStageMask;
				dependency.dstStageMask = dstStageMask;
				dependency.srcAccessMask = srcAccessMask;
				dependency.dstAccessMask = dstAccessMask;
				dependency.dependencyFlags = dependencyFlags;

				return dependency;
			}

			VkSubpassDescription createSubpassDescription(
				const std::vector<VkAttachmentReference>& colorAttachments,
				const std::vector<VkAttachmentReference>& inputAttachments,
				const VkAttachmentReference* depthStencilAttachment /* nullptr */,
				VkPipelineBindPoint bindPoint /* VK_PIPELINE_BIND_POINT_GRAPHICS */)
			{
				VkSubpassDescription newDescription = {};

				newDescription.pipelineBindPoint = bindPoint;
				newDescription.inputAttachmentCount = inputAttachments.size();
				newDescription.pInputAttachments = inputAttachments.data();
				newDescription.colorAttachmentCount = colorAttachments.size();
				newDescription.pColorAttachments = colorAttachments.data();
				newDescription.pDepthStencilAttachment = depthStencilAttachment;

				return newDescription;
			}
		}
	}
}