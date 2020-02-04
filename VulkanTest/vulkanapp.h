#include <memory>
#include <array>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "vk_mem_alloc.h"

#include "types.h"
#include "Node.h"
#include "Camera.h"
#include "Light.h"
//#include "Renderer.h"
#include "DrawlistGenerator.h"
#include "StaticMeshGenerator.h"
#include "UiDrawGenerator.h"
#include "DebugDrawGenerator.h"
#include "CubemapGenerator.h"
#include "QuadGenerator.h"
#include "CameraController.h"
#include "ModelPipeline.h"

namespace hvk {

	class VulkanApp {
	private:
		VkInstance mInstance;

		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		uint32_t mGraphicsIndex;
		VkQueue mGraphicsQueue;
		VkCommandPool mCommandPool;
		VkCommandBuffer mPrimaryCommandBuffer;

		ModelPipeline mModelPipeline;

		VkSemaphore mImageAvailable;
		VkSemaphore mRenderFinished;
		VkSemaphore mFinalRenderFinished;

		VmaAllocator mAllocator;

		VkFence mRenderFence;

	private:

		void enableVulkanValidationLayers();
		void initializeDevice();
		void initializeRenderer();

	public:
        VulkanApp();
		~VulkanApp();

		void init(
            VkInstance vulkanInstance,
            VkSurfaceKHR surface);
        bool update(double frameTime);

        ModelPipeline& getModelPipeline() { return mModelPipeline; }

		void generateEnvironmentMap(
			std::shared_ptr<TextureMap> environmentMap,
			std::shared_ptr<TextureMap> irradianceMap,
			std::shared_ptr<TextureMap> prefilteredMap,
			std::shared_ptr<TextureMap> brdfLutMap,
            const GammaSettings& gammaSettings);

		// new render paradigm
		uint32_t renderPrepare(VkSwapchainKHR& swapchain);
		VkCommandBufferInheritanceInfo renderpassBegin(const VkRenderPassBeginInfo& renderBegin);
		void renderpassExecuteAndClose(std::vector<VkCommandBuffer>& secondaryBuffers);
		void renderFinish();
		void renderSubmit();
		void renderPresent(uint32_t swapIndex, VkSwapchainKHR swapchain);
		VkCommandBuffer getPrimaryCommandBuffer() { return mPrimaryCommandBuffer; }
	};
}