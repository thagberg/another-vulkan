#pragma once

#include <memory>

#include "entt/entt.hpp"

#include "DrawlistGenerator.h"
#include "types.h"
#include "Light.h"
#include "Camera.h"
#include "PBRTypes.h"
#include "SceneTypes.h"
#include "LightTypes.h"

namespace hvk
{
	class StaticMeshGenerator : public DrawlistGenerator
	{
	private:

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorSetLayout mLightsDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mLightsDescriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;
		Resource<VkBuffer> mLightsUbo;

        HVK_shared<TextureMap> mEnvironmentMap;
		HVK_shared<TextureMap> mIrradianceMap;
		HVK_shared<TextureMap> mBrdfLutMap;

        float mGammaCorrection;
        bool mUseSRGBTex;

		void preparePipelineInfo();

	public:
        StaticMeshGenerator(
			VkRenderPass renderPass,
			VkCommandPool commandPool,
            HVK_shared<TextureMap> environmentMap,
			HVK_shared<TextureMap> irradianceMap,
			HVK_shared<TextureMap> brdfLutMap);
		virtual ~StaticMeshGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
		PBRBinding createPBRBinding(const PBRMaterial& material);

		template <typename PBRGroupType, typename LightGroupType, typename DirectionalLightType>
		VkCommandBuffer& drawElements(
			const VkCommandBufferInheritanceInfo& inheritance,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera,
			const AmbientLight& ambientLight,
			const GammaSettings& gammaSettings,
			const PBRWeight& pbrWeight,
			PBRGroupType& elements,
			LightGroupType& lights,
			DirectionalLightType& directionalLight);
	};


	template <typename PBRGroupType, typename LightGroupType, typename DirectionalLightType>
	VkCommandBuffer& StaticMeshGenerator::drawElements(
		const VkCommandBufferInheritanceInfo& inheritance,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const Camera& camera,
		const AmbientLight& ambientLight,
		const GammaSettings& gammaSettings,
		const PBRWeight& pbrWeight,
		PBRGroupType& elements,
		LightGroupType& lights,
		DirectionalLightType& directionalLight)
	{
		 // update lights
		int memOffset = 0;
		auto* copyaddr = reinterpret_cast<UniformLightObject<NUM_INITIAL_LIGHTS>*>(mLightsUbo.allocationInfo.pMappedData);
		auto uboLights = UniformLightObject<NUM_INITIAL_LIGHTS>();
        uboLights.ambient = ambientLight;
		UniformLight lightUbo = {};
		size_t i = 0;
		lights.each([&](auto entity, const auto& color, const auto& attenuation, const auto& transform) {
			lightUbo.lightPos = transform.transform[3];
			lightUbo.lightColor = color.color;
			lightUbo.lightIntensity = color.intensity;
            lightUbo.lightDirection = glm::vec3(0.f);
            lightUbo.umbra = 0.f;
            lightUbo.penumbra = 0.f;
			lightUbo.constant = attenuation.constant;
			lightUbo.linear = attenuation.linear;
			lightUbo.quadratic = attenuation.quadratic;
			uboLights.lights[i] = lightUbo;
			++i;
		});
		uboLights.numLights = static_cast<uint32_t>(i);
		auto directionalColor = std::get<0>(directionalLight);
		auto directionalDirection = std::get<1>(directionalLight);
		uboLights.directional.lightColor = directionalColor.color;
		uboLights.directional.lightIntensity = directionalColor.intensity;
		uboLights.directional.direction = directionalDirection.direction;

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
        const auto& allocator = GpuManager::getAllocator();
		elements.each([&](auto entity, const auto& mesh, const auto& binding, const auto& transform) {
			// update UBO
			vmaGetAllocationInfo(allocator, binding.ubo.allocation, &allocInfo);
			ubo.model = transform.transform;
			//ubo.modelViewProj = camera.getProjection() * camera.getViewTransform() * ubo.model;
			ubo.modelViewProj = viewProj * ubo.model;
			memcpy(allocInfo.pMappedData, &ubo, sizeof(ubo));

			vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mesh.vbo.memoryResource, offsets);
			vkCmdBindIndexBuffer(mCommandBuffer, mesh.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(
				mCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				mPipelineInfo.pipelineLayout,
				1,
				1,
				&binding.descriptorSet,
				0,
				nullptr);

			push.gamma = gammaSettings.gamma;
			push.sRGBTextures = true;
			push.pbrWeight = pbrWeight;
			vkCmdPushConstants(
				mCommandBuffer, 
				mPipelineInfo.pipelineLayout, 
				VK_SHADER_STAGE_FRAGMENT_BIT, 
				0, 
				sizeof(PushConstant), 
				&push);
			vkCmdDrawIndexed(mCommandBuffer, mesh.numIndices, 1, 0, 0, 0);
		});

		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}
}
