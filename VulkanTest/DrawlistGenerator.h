#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "types.h"
#include "GpuManager.h"

namespace hvk
{
	const uint32_t MAX_DESCRIPTORS = 20;
	const uint32_t MAX_UBOS = 40;
	const uint32_t MAX_SAMPLERS = 20;
	const uint32_t NUM_INITIAL_RENDEROBJECTS = 10;
	const uint32_t NUM_INITIAL_LIGHTS = 10;
	const uint32_t NUM_INITIAL_STATICMESHES = 10;

	struct RenderPipelineInfo 
	{
		VkPrimitiveTopology topology;
		std::string vertShaderFile;
		std::string fragShaderFile;
		VertexInfo vertexInfo;
		VkPipelineLayout pipelineLayout;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
		VkPipelineDepthStencilStateCreateInfo depthStencilState;
		VkPipelineRasterizationStateCreateInfo rasterizationState;
	};

	VkPipeline generatePipeline(
		VkRenderPass renderPass, 
		const RenderPipelineInfo& pipelineInfo);


    class DrawlistGenerator
    {
	protected:
        bool mInitialized;

        VkRenderPass mColorRenderPass;
        VkFence mRenderFence;
        VkSemaphore mRenderFinished;
        VkCommandPool mCommandPool;
        VkCommandBuffer mCommandBuffer;

        DrawlistGenerator(
			VkRenderPass renderPass,
			VkCommandPool commandPool);
        void setInitialized(bool init) { mInitialized = init; }

    public:
        virtual ~DrawlistGenerator();
		virtual void invalidate() = 0;

        bool getInitialized() const { return mInitialized; }
    };

}
