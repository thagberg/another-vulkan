#include <assert.h>

#include "imgui/imgui.h"

#include "DrawlistGenerator.h"
#define HVK_UTIL_IMPLEMENTATION
#include "vulkan-util.h"


namespace hvk
{

	VkPipeline generatePipeline(const VulkanDevice& device, VkRenderPass renderPass, const RenderPipelineInfo& pipelineInfo) {

		VkPipeline pipeline;

		auto modelVertShaderCode = readFile(pipelineInfo.vertShaderFile);
		auto modelFragShaderCode = readFile(pipelineInfo.fragShaderFile);
		VkShaderModule modelVertShaderModule = createShaderModule(device.device, modelVertShaderCode);
		VkShaderModule modelFragShaderModule = createShaderModule(device.device, modelFragShaderCode);

		VkPipelineShaderStageCreateInfo modelVertStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		modelVertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		modelVertStageInfo.module = modelVertShaderModule;
		modelVertStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo modelFragStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		modelFragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		modelFragStageInfo.module = modelFragShaderModule;
		modelFragStageInfo.pName = "main";

		std::vector<VkPipelineShaderStageCreateInfo> modelShaderStages = {
			modelVertStageInfo, modelFragStageInfo
		};

		VkPipelineInputAssemblyStateCreateInfo modelInputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		modelInputAssembly.topology = pipelineInfo.topology;
		modelInputAssembly.primitiveRestartEnable = VK_FALSE;


		pipeline = createCustomizedGraphicsPipeline(
			device.device,
			renderPass,
			pipelineInfo.pipelineLayout,
			pipelineInfo.frontFace,
			modelShaderStages,
			pipelineInfo.vertexInfo.vertexInputInfo,
			modelInputAssembly,
			pipelineInfo.depthStencilState,
			pipelineInfo.blendAttachments);

		vkDestroyShaderModule(device.device, modelVertShaderModule, nullptr);
		vkDestroyShaderModule(device.device, modelFragShaderModule, nullptr);

		return pipeline;
	}

    DrawlistGenerator::DrawlistGenerator(
        VulkanDevice device, 
        VmaAllocator allocator, 
        VkQueue graphicsQueue, 
        VkRenderPass renderPass,
        VkCommandPool commandPool) :

		mInitialized(false),
		mDevice(device),
		mAllocator(allocator),
		mGraphicsQueue(graphicsQueue),
		mRenderPass(renderPass),
		mRenderFinished(VK_NULL_HANDLE),
		mCommandPool(commandPool),
		mCommandBuffer(VK_NULL_HANDLE)
    {
		// Create fence for rendering complete
		VkFenceCreateInfo fenceCreate = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreate.pNext = nullptr;
		fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		assert(vkCreateFence(mDevice.device, &fenceCreate, nullptr, &mRenderFence) == VK_SUCCESS);

		// create semaphore for rendering finished
		mRenderFinished = createSemaphore(mDevice.device);

		// Allocate command buffer
		VkCommandBufferAllocateInfo bufferAlloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		bufferAlloc.commandBufferCount = 1;
		bufferAlloc.commandPool = mCommandPool;
		bufferAlloc.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

		assert(vkAllocateCommandBuffers(mDevice.device, &bufferAlloc, &mCommandBuffer) == VK_SUCCESS);
    }

    DrawlistGenerator::~DrawlistGenerator()
    {
    }
}
