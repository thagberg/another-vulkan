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
#include "CameraController.h"

namespace hvk {

	class VulkanApp {
	private:
		uint32_t mSurfaceWidth, mSurfaceHeight;
		VkInstance mInstance;
		VkSurfaceKHR mSurface;

		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		uint32_t mGraphicsIndex;
		VkQueue mGraphicsQueue;
		VkRenderPass mRenderPass;
		VkCommandPool mCommandPool;
		VkCommandBuffer mPrimaryCommandBuffer;

		SwapchainImageViews mImageViews;
		SwapchainImages mSwapchainImages;
		Resource<VkImage> mDepthResource;
		VkImageView mDepthView;
		FrameBuffers mFramebuffers;
		hvk::Swapchain mSwapchain;

		VkSemaphore mImageAvailable;
		VkSemaphore mRenderFinished;

		VmaAllocator mAllocator;

		CameraRef mActiveCamera;

		std::shared_ptr<StaticMeshGenerator> mMeshRenderer;
		std::shared_ptr<UiDrawGenerator> mUiRenderer;
		std::shared_ptr<DebugDrawGenerator> mDebugRenderer;

		VkFence mRenderFence;

		void enableVulkanValidationLayers();
		void initializeDevice();
		void initializeRenderer();
		void initFramebuffers();
		void drawFrame();
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

        void addStaticMeshInstance(HVK_shared<StaticMeshRenderObject> node);
        void addDynamicLight(HVK_shared<Light> light);
        void addDebugMeshInstance(HVK_shared<DebugMeshRenderObject> node);
        void setActiveCamera(HVK_shared<Camera> camera);
		void recreateSwapchain(uint32_t surfaceWidth, uint32_t surfaceHeight);

        void setGammaCorrection(float gamma);
        void setUseSRGBTex(bool useSRGBTex);
        float getGammaCorrection() { return mMeshRenderer->getGammaCorrection(); }
        bool isUseSRGBTex() { return mMeshRenderer->isUseSRGBTex(); }
	};
}