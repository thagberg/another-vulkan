#pragma once

#include <memory>

#include "DrawlistGenerator.h"
#include "RenderObject.h"
#include "types.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "DebugDrawTypes.h"

namespace hvk
{

	class DebugDrawGenerator : public DrawlistGenerator
	{
	private:
		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

	public:
		DebugDrawGenerator(
			VkRenderPass renderPass,
			VkCommandPool commandPool);
		virtual ~DebugDrawGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
		DebugDrawBinding createDebugDrawBinding();

		template <typename DebugDrawGroupType>
		VkCommandBuffer& drawElements(
			const VkCommandBufferInheritanceInfo& inheritance,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera,
			DebugDrawGroupType& elements);
	};

	template <typename DebugDrawGroupType>
	VkCommandBuffer& DebugDrawGenerator::drawElements(
		const VkCommandBufferInheritanceInfo& inheritance,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const Camera& camera,
		DebugDrawGroupType& elements)
	{
		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		commandBegin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBegin.pInheritanceInfo = &inheritance;

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
		vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);
		VkDeviceSize offsets[] = { 0 };

		auto viewProj = camera.getProjection() * camera.getViewTransform();
		UniformBufferObject ubo = {
			glm::mat4(1.f),
			camera.getViewTransform(),
			viewProj,
			camera.getWorldPosition()
		};
		VmaAllocationInfo allocInfo;
		const auto& allocator = GpuManager::getAllocator();

		elements.each([&](auto entity, const auto& mesh, const auto& binding, const auto& transform) {
			vmaGetAllocationInfo(allocator, binding.ubo.allocation, &allocInfo);
			ubo.model = transform.transform;
			ubo.model[1][1] *= -1;
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

