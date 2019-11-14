#pragma once

#include "DrawlistGenerator.h"
#include "Camera.h"
#include "types.h"

namespace hvk
{
	class SkyboxGenerator : public DrawlistGenerator
	{
	private:
		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet descriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

		TextureMap mSkyboxMap;
		
	public:
		SkyboxGenerator(
			VulkanDevice device,
			VmaAllocator allocator,
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool);
		virtual ~SkyboxGenerator();
		virtual void invalidate() override;
		VkCommandBuffer& drawFrame(
			const VkCommandBufferInheritanceInfo& inheritance,
			const VkFramebuffer& framebuffer,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera);
	};
}
