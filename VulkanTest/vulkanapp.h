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
		window_ptr mWindow;
		VkInstance mInstance;
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		uint32_t mGraphicsIndex;
		VkQueue mGraphicsQueue;
		VkSurfaceKHR mSurface;
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

		CameraRef mCameraNode;
		CameraController mCameraController;
		//NodeRef mLightBox;

		//Renderer mRenderer;
		std::shared_ptr<StaticMeshGenerator> mMeshRenderer;
		std::shared_ptr<UiDrawGenerator> mUiRenderer;
		std::shared_ptr<DebugDrawGenerator> mDebugRenderer;

		VkFence mRenderFence;

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
        bool update(double frameTime);

        void toggleCursor(bool enabled);
        void addStaticMeshInstance(HVK_shared<StaticMeshRenderObject> node);
        void addDynamicLight(HVK_shared<Light> light);
        void addDebugMeshInstance(HVK_shared<DebugMeshRenderObject> node);

		window_ptr getWindow() { 
			return mWindow; 
		}
	};
}