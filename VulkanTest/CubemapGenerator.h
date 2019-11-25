#pragma once

#include "DrawlistGenerator.h"
#include "Camera.h"
#include "RenderObject.h"
#include "CubeMesh.h"
#include "types.h"

namespace hvk
{
	class CubemapGenerator : public DrawlistGenerator
	{
	private:
		struct CubemapRenderable
		{
			HVK_shared<CubeMeshRenderObject> renderObject;

			Resource<VkBuffer> vbo;
			Resource<VkBuffer> ibo;
			size_t numVertices;
			uint32_t numIndices;

			Resource<VkBuffer> ubo;
			bool sRGB;
		};

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mDescriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

		HVK_shared<TextureMap> mCubeMap;
		CubemapRenderable mCubeRenderable;
		
	public:
		CubemapGenerator(
			VulkanDevice device,
			VmaAllocator allocator,
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool,
            HVK_shared<TextureMap> skyboxMap,
			std::array<std::string, 2>& shaderFiles);
		virtual ~CubemapGenerator();
		virtual void invalidate() override;
		VkCommandBuffer& drawFrame(
			const VkCommandBufferInheritanceInfo& inheritance,
			const VkFramebuffer& framebuffer,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera,
			const GammaSettings& gammaSettings);
		void updateRenderPass(VkRenderPass renderPass);
	};
}
