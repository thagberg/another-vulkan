#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "types.h"

namespace hvk
{
	namespace util
	{
		namespace pipeline
		{
			VkPipelineDepthStencilStateCreateInfo createDepthStencilState(
				bool depthTest = VK_TRUE,
				bool depthWrite = VK_TRUE,
				bool stencilTest = VK_FALSE,
				float minDepthBounds = 0.f,
				float maxDepthBounds = 1.f,
				VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL);

			VkPipelineRasterizationStateCreateInfo createRasterizationState(
				VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL,
				VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT);

			VkPipeline createGraphicsPipeline(
				VkDevice device,
				VkRenderPass renderPass,
				VkPipelineLayout pipelineLayout,
				const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
				const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
				const VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
				const VkPipelineDepthStencilStateCreateInfo& depthStencilInfo,
				const VkPipelineRasterizationStateCreateInfo& rasterizationInfo,
				const std::vector<VkPipelineColorBlendAttachmentState>& blendAttachments);

			template <typename T>
			void fillVertexInfo(VertexInfo& vertexInfo) 
			{
				vertexInfo.bindingDescription = T::getBindingDescription();
				vertexInfo.attributeDescriptions = T::getAttributeDescriptions();
				vertexInfo.vertexInputInfo = {
					VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
					nullptr,
					0,
					1,
					&vertexInfo.bindingDescription,
					static_cast<uint32_t>(vertexInfo.attributeDescriptions.size()),
					vertexInfo.attributeDescriptions.data()
				};
			}
		}
	}
}
