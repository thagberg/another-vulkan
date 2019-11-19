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

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(QuadVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
			attributeDescriptions.resize(2);

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(QuadVertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(QuadVertex, uv);


			return attributeDescriptions;
		}
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
        VkCommandBuffer& drawFrame(
            const VkCommandBufferInheritanceInfo& inheritance,
            const VkFramebuffer& framebuffer,
            const VkViewport& viewport,
            const VkRect2D& scissor);
	};
}
