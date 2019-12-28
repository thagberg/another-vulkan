#pragma once

#include "DrawlistGenerator.h"
#include "types.h"

namespace hvk
{
	class UiDrawGenerator : public DrawlistGenerator
	{
	private:
		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mDescriptorSet;
        RuntimeResource<VkImage> mFontImage;
		VkImageView mFontView;
		VkSampler mFontSampler;
		Resource<VkBuffer> mVbo;
		Resource<VkBuffer> mIbo;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;
		VkExtent2D mWindowExtent;
	public:
		UiDrawGenerator(
			VulkanDevice device,
			VmaAllocator  allocator,
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool,
			VkExtent2D windowExtent);
		virtual ~UiDrawGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass, VkExtent2D windowExtent);
		VkCommandBuffer& drawFrame(
            const VkCommandBufferInheritanceInfo& inheritance,
			VkFramebuffer& framebuffer,
			const VkViewport& viewport,
			const VkRect2D& scissor);
	};
}
