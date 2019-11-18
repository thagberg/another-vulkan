#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "types.h"

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
		const char* vertShaderFile;
		const char* fragShaderFile;
		VertexInfo vertexInfo;
		VkPipelineLayout pipelineLayout;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
		VkFrontFace frontFace;
		VkPipelineDepthStencilStateCreateInfo depthStencilState;
	};

	VkPipeline generatePipeline(
		const VulkanDevice& device, 
		VkRenderPass renderPass, 
		const RenderPipelineInfo& pipelineInfo);


    class DrawlistGenerator
    {
	protected:
        bool mInitialized;

        VulkanDevice mDevice;
        VmaAllocator mAllocator;
        VkQueue mGraphicsQueue;

        VkRenderPass mRenderPass;
        VkFence mRenderFence;
        VkSemaphore mRenderFinished;
        VkCommandPool mCommandPool;
        VkCommandBuffer mCommandBuffer;

        DrawlistGenerator(
			VulkanDevice device, 
			VmaAllocator allocator, 
			VkQueue mGraphicsQueue, 
			VkRenderPass renderPass,
			VkCommandPool commandPool);
        void setInitialized(bool init) { mInitialized = init; }

    public:
        virtual ~DrawlistGenerator();
		virtual void invalidate() = 0;

        bool getInitialized() const { return mInitialized; }
    };

}
