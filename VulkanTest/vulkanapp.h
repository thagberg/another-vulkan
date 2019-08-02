#include <memory>
#include <array>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "types.h"
#include "Node.h"
#include "Camera.h"
#include "Renderer.h"

namespace hvk {

	class VulkanApp {
	private:
		window_ptr mWindow;
		VkInstance mInstance;
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		uint32_t mGraphicsIndex;
		VkQueue mGraphicsQueue;
		VkSurfaceKHR mSurface;
		VkRenderPass mRenderPass;
		VkCommandPool mCommandPool;

		SwapchainImageViews mImageViews;
		SwapchainImages mSwapchainImages;
		FrameBuffers mFramebuffers;
		hvk::Swapchain mSwapchain;

		VkSemaphore mImageAvailable;
		VkSemaphore mRenderFinished;

        NodeRef mObjectNode;
		CameraRef mCameraNode;

		Renderer mRenderer;

		int mWindowWidth, mWindowHeight;

		void initializeVulkan();
		void enableVulkanValidationLayers();
		void initializeDevice();
		void initializeRenderer();
		void drawFrame();

	public:
		VulkanApp(int width, int height, const char* windowTitle);
		~VulkanApp();

		void init();
		void run();
        void processKeyInput(int keyCode, bool pressed);

		window_ptr getWindow() { 
			return mWindow; 
		}
	};
}