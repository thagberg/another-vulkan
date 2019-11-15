#pragma once

#include "DrawlistGenerator.h"
#include "Camera.h"
#include "RenderObject.h"
#include "CubeMesh.h"
#include "types.h"

namespace hvk
{
	class SkyboxGenerator : public DrawlistGenerator
	{
	private:
		struct SkyboxRenderable
		{
			HVK_shared<CubeMeshRenderObject> renderObject;

			Resource<VkBuffer> vbo;
			Resource<VkBuffer> ibo;
			size_t numVertices;
			uint32_t numIndices;

			Resource<VkBuffer> ubo;
		};

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mDescriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

		TextureMap mSkyboxMap;
		SkyboxRenderable mSkyboxRenderable;
		
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
		void updateRenderPass(VkRenderPass renderPass);
	};
}
