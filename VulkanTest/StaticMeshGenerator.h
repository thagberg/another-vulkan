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
			uint32_t numIndices;

            TextureMap diffuseMap;
            TextureMap metallicRoughnessMap;
            TextureMap normalMap;

			Resource<VkBuffer> ubo;
			VkDescriptorSet descriptorSet;
		};

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorSetLayout mLightsDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mLightsDescriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;
		Resource<VkBuffer> mLightsUbo;

		HVK_vector<StaticMeshRenderable> mRenderables;
		HVK_vector<HVK_shared<Light>> mLights;

        HVK_shared<TextureMap> mEnvironmentMap;
		HVK_shared<TextureMap> mIrradianceMap;

        float mGammaCorrection;
        bool mUseSRGBTex;

		void preparePipelineInfo();

	public:
        StaticMeshGenerator(
			VulkanDevice device, 
			VmaAllocator allocator, 
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool,
            HVK_shared<TextureMap> environmentMap,
			HVK_shared<TextureMap> irradianceMap);
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
			const AmbientLight& ambientLight,
			const GammaSettings& gammaSettings,
			const PBRWeight& pbrWeight);
        void setGammaCorrection(float gamma);
        void setUseSRGBTex(bool useSRGBTex);
        float getGammaCorrection() { return mGammaCorrection; }
        bool isUseSRGBTex() { return mUseSRGBTex; }
	};
}
