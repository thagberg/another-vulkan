#pragma once

#include <memory>

#include "DrawlistGenerator.h"
#include "RenderObject.h"
#include "types.h"
#include "ResourceManager.h"
#include "Camera.h"


namespace hvk
{

	class DebugDrawGenerator : public DrawlistGenerator
	{
	private:
		struct DebugMeshRenderable
		{
			std::shared_ptr<DebugMeshRenderObject> renderObject;

			Resource<VkBuffer> vbo;
			Resource<VkBuffer> ibo;
			size_t numVertices;
			uint32_t numIndices;

			Resource<VkBuffer> ubo;
			VkDescriptorSet descriptorSet;
		};

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

		std::vector<DebugMeshRenderable, Hallocator<DebugMeshRenderable>> mRenderables;

	public:
		DebugDrawGenerator(
			VulkanDevice device,
			VmaAllocator allocator,
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool);
		virtual ~DebugDrawGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
		void addDebugMeshObject(std::shared_ptr<DebugMeshRenderObject> object);
		VkCommandBuffer& drawFrame(
			const VkCommandBufferInheritanceInfo& inheritance,
			const VkFramebuffer& framebuffer,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera);
	};
}

