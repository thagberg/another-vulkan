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
#include "Renderer.h"
#include "Clock.h"
#include "CameraController.h"

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
		Resource<VkImage> mDepthResource;
		VkImageView mDepthView;
		FrameBuffers mFramebuffers;
		hvk::Swapchain mSwapchain;

		VkSemaphore mImageAvailable;
		VkSemaphore mRenderFinished;

		VmaAllocator mAllocator;

        NodeRef mObjectNode;
		CameraRef mCameraNode;
		CameraController mCameraController;
		LightRef mLightNode;

		Renderer mRenderer;

		Clock mClock;

		double mLastX, mLastY;
		bool mMouseLeftDown;

		int mWindowWidth, mWindowHeight;

		void initializeVulkan();
		void enableVulkanValidationLayers();
		void initializeDevice();
		void initializeRenderer();
		void initializeApp();
		void initFramebuffers();
		void drawFrame();
		void cleanupSwapchain();

		static void handleWindowResize(GLFWwindow* window, int width, int height);
		void recreateSwapchain();

		/*-- Things which should be handled elsewhere later --*/
		//void updateCamera(double deltaT);

	public:
		VulkanApp(int width, int height, const char* windowTitle);
		~VulkanApp();

		void init();
		void run();

		window_ptr getWindow() { 
			return mWindow; 
		}
	};
}