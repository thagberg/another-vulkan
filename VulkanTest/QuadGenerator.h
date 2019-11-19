#pragma once
#include "DrawlistGenerator.h"
#include "types.h"

namespace hvk
{
	const uint32_t numVertices = 4;
	const uint16_t numIndices = 6;

	struct QuadVertex {
		glm::vec2 pos;
		glm::vec2 uv;
	};

	const std::array<QuadVertex, 4> quadVertices = {
		QuadVertex{glm::vec2(-1.f, -1.f), glm::vec2(0.f, 0.f)},
		QuadVertex{glm::vec2(1.f, -1.f), glm::vec2(1.f, 0.f)},
		QuadVertex{glm::vec2(-1.f, 1.f), glm::vec2(0.f, 1.f)},
		QuadVertex{glm::vec2(1.f, 1.f), glm::vec2(1.f, 1.f)},
	};

	const std::array<uint16_t, 6> quadIndices = {
		0,1,2,
		1,2,3
	};

	class QuadGenerator : public DrawlistGenerator
	{
	private:
		struct QuadRenderable
		{
			Resource<VkBuffer> vbo;
			Resource<VkBuffer> ibo;
		};

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mDescriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

		QuadRenderable mRenderable;
		HVK_shared<TextureMap> mOffscreenMap;

	public:
		QuadGenerator(
			VulkanDevice device,
			VmaAllocator allocator,
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool,
			HVK_shared<TextureMap> offscreenMap);
		virtual ~QuadGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
	};
}
