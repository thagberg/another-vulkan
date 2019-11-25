#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

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

			VkPipeline createGraphicsPipeline(
				VkDevice device,
				VkRenderPass renderPass,
				VkPipelineLayout pipelineLayout,
				VkFrontFace frontFace,
				const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
				const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
				const VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
				const VkPipelineDepthStencilStateCreateInfo& depthStencilInfo,
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
