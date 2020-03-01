#include <assert.h>

#include "imgui/imgui.h"

#include "DrawlistGenerator.h"
#include "pipeline-util.h"
#include "signal-util.h"
#include "vulkan-util.h"


namespace hvk
{
	VkPipeline generatePipeline(VkRenderPass renderPass, const RenderPipelineInfo& pipelineInfo) {

		VkPipeline pipeline;

        const VkDevice& device = GpuManager::getDevice();

		auto modelVertShaderCode = readFile(pipelineInfo.vertShaderFile);
		auto modelFragShaderCode = readFile(pipelineInfo.fragShaderFile);
		VkShaderModule modelVertShaderModule = createShaderModule(device, modelVertShaderCode);
		VkShaderModule modelFragShaderModule = createShaderModule(device, modelFragShaderCode);

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


		pipeline = util::pipeline::createGraphicsPipeline(
			device,
			renderPass,
			pipelineInfo.pipelineLayout,
			modelShaderStages,
			pipelineInfo.vertexInfo.vertexInputInfo,
			modelInputAssembly,
			pipelineInfo.depthStencilState,
			pipelineInfo.rasterizationState,
			pipelineInfo.blendAttachments);

		vkDestroyShaderModule(device, modelVertShaderModule, nullptr);
		vkDestroyShaderModule(device, modelFragShaderModule, nullptr);

		return pipeline;
	}

    DrawlistGenerator::DrawlistGenerator(
        VkRenderPass renderPass,
        VkCommandPool commandPool) :

		mInitialized(false),
		mColorRenderPass(renderPass),
		mRenderFinished(VK_NULL_HANDLE),
		mCommandPool(commandPool),
		mCommandBuffer(VK_NULL_HANDLE)
    {
        const VkDevice device = GpuManager::getDevice();

		// Create fence for rendering complete
		VkFenceCreateInfo fenceCreate = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreate.pNext = nullptr;
		fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		assert(vkCreateFence(device, &fenceCreate, nullptr, &mRenderFence) == VK_SUCCESS);

		// create semaphore for rendering finished
		mRenderFinished = util::signal::createSemaphore(device);

		// Allocate command buffer
		VkCommandBufferAllocateInfo bufferAlloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		bufferAlloc.commandBufferCount = 1;
		bufferAlloc.commandPool = mCommandPool;
		bufferAlloc.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

		assert(vkAllocateCommandBuffers(device, &bufferAlloc, &mCommandBuffer) == VK_SUCCESS);
    }

    DrawlistGenerator::~DrawlistGenerator()
    {
    }
}
