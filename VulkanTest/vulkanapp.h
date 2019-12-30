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
		uint32_t mSurfaceWidth, mSurfaceHeight;
		VkInstance mInstance;
		VkSurfaceKHR mSurface;	// M

		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		uint32_t mGraphicsIndex;
		VkQueue mGraphicsQueue;
		VkRenderPass mColorRenderPass;	// M
        VkRenderPass mFinalRenderPass; // M
		VkCommandPool mCommandPool;
		VkCommandBuffer mPrimaryCommandBuffer;

		ModelPipeline mModelPipeline;

		SwapchainImageViews mFinalPassImageViews; // M
		SwapchainImages mFinalPassSwapchainImages; // M
        HVK_shared<TextureMap> mColorPassMap; // M
		Resource<VkImage> mDepthResource; // M
		VkImageView mDepthView; // M
		FrameBuffers mFinalPassFramebuffers; // M
        VkFramebuffer mColorPassFramebuffer; // M
		Swapchain mSwapchain; // M

		VkSemaphore mImageAvailable;
		VkSemaphore mRenderFinished;
		VkSemaphore mFinalRenderFinished;

		VmaAllocator mAllocator;

		CameraRef mActiveCamera;	// M

		VkFence mRenderFence;

	private:

		void enableVulkanValidationLayers();
		void initializeDevice();
		void initializeRenderer();
		void initFramebuffers();
		void cleanupSwapchain();

	public:
        VulkanApp();
		~VulkanApp();

		void init(
            uint32_t surfaceWidth, 
            uint32_t surfaceHeight,
            VkInstance vulkanInstance,
            VkSurfaceKHR surface);
        bool update(double frameTime);

        ModelPipeline& getModelPipeline() { return mModelPipeline; }

        void setActiveCamera(HVK_shared<Camera> camera);
		void recreateSwapchain(uint32_t surfaceWidth, uint32_t surfaceHeight);

        void setGammaCorrection(float gamma);
        void setUseSRGBTex(bool useSRGBTex);
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
	};
}