#pragma once
#include "DrawlistGenerator.h"

namespace hvk
{
	struct ShadowBinding
	{
		RuntimeResource<VkBuffer> ubo;
		VkDescriptorSet descriptorSet;
	};

	class ShadowGenerator : public DrawlistGenerator
	{
	private:
		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

		void preparePipelineInfo();

	public:
		ShadowGenerator(
			VkRenderPass renderPass,
			VkCommandPool commandPool);
		virtual ~ShadowGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
		ShadowBinding createBinding();

		template <typename ShadowGroupType, typename LightType>
		VkCommandBuffer& drawElements(
			const VkCommandBufferInheritanceInfo& inheritance,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const LightType& light,
			const ShadowGroupType& shadowables);
	};

	template <typename ShadowGroupType, typename LightType>
	VkCommandBuffer& ShadowGenerator::drawElements(
		const VkCommandBufferInheritanceInfo& inheritance,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const LightType& light,
		const ShadowGroupType& shadowables)
	{
		const auto& device = GpuManager::getDevice();
		const auto& allocator = GpuManager::getAllocator();

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		commandBegin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBegin.pInheritanceInfo = &inheritance;
		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		auto worldTransform = std::get<0>(light).transform;
		auto viewTransform = glm::inverse(worldTransform);
		auto projection = std::get<1>(light).projection;
		auto viewProj = projection * viewTransform;
		UniformBufferObject ubo = {
			glm::mat4(1.f),
			viewTransform,
			viewProj,
			worldTransform[3]
		};

		VmaAllocationInfo allocInfo;
		shadowables.each([&](auto entity, const auto& mesh, const auto& binding, const auto& transform) {
			// update UBO
			vmaGetAllocationInfo(allocator, binding.ubo.allocation, &allocInfo);
			ubo.model = transform.transform;
			ubo.modelViewProj = viewProj * ubo.model;
			memcpy(allocInfo.pMappedData, &ubo, sizeof(ubo));

			vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mesh.vbo.memoryResource, offsets);
			vkCmdBindIndexBuffer(mCommandBuffer, mesh.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(
				mCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				mPipelineInfo.pipelineLayout,
				0,
				1,
				&binding.descriptorSet,
				0,
				nullptr);

			vkCmdDrawIndexed(mCommandBuffer, mesh.numIndices, 1, 0, 0, 0);
		});

		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}
}
