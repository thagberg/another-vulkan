#pragma once

#include <memory>

#include "entt/entt.hpp"

#include "DrawlistGenerator.h"
#include "types.h"
#include "RenderObject.h"
#include "ResourceManager.h"
#include "Light.h"
#include "Camera.h"
#include "PBRTypes.h"

namespace hvk
{
	class NodeTransform;

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
		HVK_shared<TextureMap> mBrdfLutMap;

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
			HVK_shared<TextureMap> irradianceMap,
			HVK_shared<TextureMap> brdfLutMap);
		virtual ~StaticMeshGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
		void addStaticMeshObject(std::shared_ptr<StaticMeshRenderObject> object);
		PBRBinding createPBRBinding(const PBRMaterial& material);
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

		template <typename PBRGroupType>
		VkCommandBuffer& drawElements(
			const VkCommandBufferInheritanceInfo& inheritance,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera,
			const AmbientLight& ambientLight,
			const GammaSettings& gammaSettings,
			const PBRWeight& pbrWeight,
			PBRGroupType& elements);
	};


	template <typename PBRGroupType>
	VkCommandBuffer& StaticMeshGenerator::drawElements(
		const VkCommandBufferInheritanceInfo& inheritance,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const Camera& camera,
		const AmbientLight& ambientLight,
		const GammaSettings& gammaSettings,
		const PBRWeight& pbrWeight,
		PBRGroupType& elements)
	{
		 // update lights
		int memOffset = 0;
		auto* copyaddr = reinterpret_cast<UniformLightObject<NUM_INITIAL_LIGHTS>*>(mLightsUbo.allocationInfo.pMappedData);
		auto uboLights = UniformLightObject<NUM_INITIAL_LIGHTS>();
		uboLights.numLights = static_cast<uint32_t>(mLights.size());
        uboLights.ambient = ambientLight;
		for (size_t i = 0; i < mLights.size(); ++i) {
			HVK_shared<Light> light = mLights[i];
			UniformLight ubo = {};
			ubo.lightPos = light->getWorldPosition();
			ubo.lightColor = light->getColor();
			ubo.lightIntensity = light->getIntensity();
			uboLights.lights[i] = ubo;
		}
		memcpy(copyaddr, &uboLights, sizeof(uboLights));

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBegin.pInheritanceInfo = &inheritance;

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		// bind viewport and scissor
		vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		// bind lights descriptor set to set 0
		vkCmdBindDescriptorSets(
			mCommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			mPipelineInfo.pipelineLayout,
			0,
			1,
			&mLightsDescriptorSet,
			0,
			nullptr);

		auto viewProj = camera.getProjection() * camera.getViewTransform();
		UniformBufferObject ubo = {
			glm::mat4(1.f),
			camera.getViewTransform(),
			viewProj,
			camera.getWorldPosition()
		};

		// Prepare and draw PBR elements
		PushConstant push = {};
		VmaAllocationInfo allocInfo;
		elements.each([](auto entity, auto transform, const auto& mesh, const auto& binding) {
			// update UBO
			auto allocInfo = vmaGetAllocationInfo(mAllocator, binding.ubo.allocation, , &allocInfo);
			ubo.model = transform.localTransform;
			ubo.model[1][1] *= -1;
			ubo.modelViewProj = viewProj * ubo.model;
			memcpy(allocInfo.pMappedData, &ubo, sizeof(ubo));

			vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, binding.vbo.memoryResource, offsets);
			vkCmdBindIndexBuffer(mCommandBuffer, binding.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(
				mCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAHPICS,
				mPipelineInfo.pipelineLayout,
				1,
				1,
				&binding.descriptorSet,
				0,
				nullptr);

			push.gamma = gammaSettings.gamma;
			push.sRGBTextures = true;
			push.pbrWeight = pbrWeight;
			vkCmdPushConstants(mCommandBuffer, mPipelineInfo.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &push);
			vkCmdDrawIndexed(mCommandBuffer, mesh.numIndices, 1, 0, 0, 0);
		});

		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}
}
