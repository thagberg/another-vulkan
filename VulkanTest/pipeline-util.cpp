#include "pch.h"
#include "pipeline-util.h"

namespace hvk
{
	namespace util
	{
		namespace pipeline
		{
			VkPipelineDepthStencilStateCreateInfo createDepthStencilState (
				bool depthTest,
				bool depthWrite,
				bool stencilTest,
				float minDepthBounds,
				float maxDepthBounds,
				VkCompareOp depthCompareOp)
			{
				VkPipelineDepthStencilStateCreateInfo depthCreate = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
				depthCreate.depthTestEnable = depthTest;
				depthCreate.depthWriteEnable = depthWrite;
				depthCreate.stencilTestEnable = stencilTest;
				depthCreate.depthCompareOp = depthCompareOp;
				depthCreate.depthBoundsTestEnable = VK_FALSE;
				depthCreate.minDepthBounds = minDepthBounds;
				depthCreate.maxDepthBounds = maxDepthBounds;

				return depthCreate;
			}


			VkPipeline createGraphicsPipeline(
				VkDevice device,
				VkRenderPass renderPass,
				VkPipelineLayout pipelineLayout,
				VkFrontFace frontFace,
				const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
				const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
				const VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
				const VkPipelineDepthStencilStateCreateInfo& depthStencilInfo,
				const std::vector<VkPipelineColorBlendAttachmentState>& blendAttachments) {

				VkPipeline graphicsPipeline;

				VkPipelineViewportStateCreateInfo viewportState = {};
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.viewportCount = 1;
				viewportState.pViewports = nullptr;
				viewportState.scissorCount = 1;
				viewportState.pScissors = nullptr;

				VkPipelineRasterizationStateCreateInfo rasterizer = {};
				rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizer.depthClampEnable = VK_FALSE;
				rasterizer.rasterizerDiscardEnable = VK_FALSE;
				rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
				rasterizer.lineWidth = 1.0f;
				rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
				rasterizer.frontFace = frontFace;
				//rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				rasterizer.depthBiasEnable = VK_FALSE;
				rasterizer.depthBiasConstantFactor = 0.0f;
				rasterizer.depthBiasClamp = 0.0f;
				rasterizer.depthBiasSlopeFactor = 0.0f;

				VkPipelineMultisampleStateCreateInfo multisampling = {};
				multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampling.sampleShadingEnable = VK_FALSE;
				multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				multisampling.minSampleShading = 1.0f;
				multisampling.pSampleMask = nullptr;
				multisampling.alphaToCoverageEnable = VK_FALSE;
				multisampling.alphaToOneEnable = VK_FALSE;

				VkPipelineColorBlendStateCreateInfo colorBlending = {};
				colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlending.logicOpEnable = VK_FALSE;
				colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
				colorBlending.pAttachments = blendAttachments.data();

				std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

				VkPipelineDynamicStateCreateInfo dynamicCreate = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
				dynamicCreate.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
				dynamicCreate.pDynamicStates = dynamicStates.data();

				VkGraphicsPipelineCreateInfo pipelineInfo = {};
				pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
				pipelineInfo.pStages = shaderStages.data();
				pipelineInfo.pVertexInputState = &vertexInputInfo;
				pipelineInfo.pInputAssemblyState = &inputAssembly;
				pipelineInfo.pViewportState = &viewportState;
				pipelineInfo.pRasterizationState = &rasterizer;
				pipelineInfo.pMultisampleState = &multisampling;
				pipelineInfo.pDepthStencilState = &depthStencilInfo;
				pipelineInfo.pColorBlendState = &colorBlending;
				pipelineInfo.pDynamicState = &dynamicCreate;
				pipelineInfo.layout = pipelineLayout;
				pipelineInfo.renderPass = renderPass;
				pipelineInfo.subpass = 0;
				pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
				pipelineInfo.basePipelineIndex = -1;

				assert(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) == VK_SUCCESS);

				return graphicsPipeline;
			}
		}
	}
}
