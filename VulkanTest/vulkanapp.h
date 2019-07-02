#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace hvk {

	typedef std::vector<VkImageView> SwapchainImageViews;
	typedef std::vector<VkImage> SwapchainImages;
	typedef std::vector<VkFramebuffer> FrameBuffers;
	typedef std::vector<VkCommandBuffer> CommandBuffers;
	//typedef std::shared_ptr<GLFWwindow, void(*)(GLFWwindow*)> window_ptr;
	typedef std::shared_ptr<GLFWwindow> window_ptr;

	struct Swapchain {
		VkSwapchainKHR swapchain;
		VkFormat swapchainImageFormat;
		VkExtent2D swapchainExtent;
	};

	class VulkanApp {
	private:
		VkInstance mInstance;
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;
		VkQueue mGraphicsQueue;
		VkPipelineLayout mPipelineLayout;
		VkPipeline mGraphicsPipeline;
		VkSurfaceKHR mSurface;
		VkRenderPass mRenderPass;
		VkCommandPool mCommandPool;
		CommandBuffers mCommandBuffers;
		SwapchainImageViews mImageViews;
		SwapchainImages mSwapchainImages;
		FrameBuffers mFramebuffers;
		hvk::Swapchain mSwapchain;
		VkSemaphore mImageAvailable;
		VkSemaphore mRenderFinished;
		uint32_t mGraphicsIndex;
		window_ptr mWindow;
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

		window_ptr getWindow() { 
			return mWindow; 
		}
	};
}