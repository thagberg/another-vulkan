#pragma once

#include <memory>

#include "DrawlistGenerator.h"
#include "types.h"
#include "RenderObject.h"
#include "ResourceManager.h"
#include "Light.h"
#include "Camera.h"

namespace hvk
{
	class StaticMeshGenerator : public DrawlistGenerator
	{
	private:

		struct StaticMeshRenderable 
		{
			std::shared_ptr<StaticMeshRenderObject> renderObject;

			Resource<VkBuffer> vbo;
			Resource<VkBuffer> ibo;
			size_t numVertices;
			uint16_t numIndices;

			Resource<VkImage> texture;
			VkImageView textureView;
			VkSampler textureSampler;
			Resource<VkBuffer> ubo;
			VkDescriptorSet descriptorSet;
		};

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorSetLayout mLightsDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mLightsDescriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;
		Resource<VkBuffer> mLightsUbo;						// should this be a ptr?

		std::vector<StaticMeshRenderable, Hallocator<StaticMeshRenderable>> mRenderables;
		//std::vector<StaticMeshRenderable> mRenderables;
		std::vector<std::shared_ptr<Light>> mLights;

		void preparePipelineInfo();

	public:
        StaticMeshGenerator(
			VulkanDevice device, 
			VmaAllocator allocator, 
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool);
		virtual ~StaticMeshGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
		void addStaticMeshObject(std::shared_ptr<StaticMeshRenderObject> object);
		void addLight(std::shared_ptr<Light> light);
		VkCommandBuffer& drawFrame(
            const VkCommandBufferInheritanceInfo& inheritance,
			const VkFramebuffer& framebuffer,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera,
			const AmbientLight& ambientLight);
	};
}
