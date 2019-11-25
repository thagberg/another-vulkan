#include "pch.h"
#include "framebuffer-util.h"

#include <vector>
#include <assert.h>

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
				const VkImageView& imageView,
				const VkImageView* pDepthView,
				VkFramebuffer* oFramebuffer)
			{
				std::vector<VkImageView> attachments;
				attachments.push_back(imageView);
				if (pDepthView != nullptr) {
					attachments.push_back(*pDepthView);
				}

				VkFramebufferCreateInfo fb = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
				fb.renderPass = renderPass;
				fb.attachmentCount = static_cast<uint32_t>(attachments.size());
				fb.pAttachments = attachments.data();
				fb.width = extent.width;
				fb.height = extent.height;
				fb.layers = 1;

				assert(vkCreateFramebuffer(device, &fb, nullptr, oFramebuffer) == VK_SUCCESS);
			}
		}
	}
}