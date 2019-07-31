#include <memory>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "vk_mem_alloc.h"

#include "types.h"
#include "Node.h"
#include "Camera.h"

namespace hvk {

	class VulkanApp {
	private:
		window_ptr mWindow;
		VkInstance mInstance;
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		uint32_t mGraphicsIndex;
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

		VmaAllocator mAllocator;
		hvk::Resource<VkImage> mTexture;
		hvk::Resource<VkBuffer> mVertexBufferResource;
		hvk::Resource<VkBuffer> mIndexBufferResource;
		UniformBufferResources mUniformBufferResources;

		VkImageView mTextureView;
        VkSampler mTextureSampler;

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		DescriptorSets mDescriptorSets;

        NodeRef mObjectNode;
		CameraRef mCameraNode;

		int mWindowWidth, mWindowHeight;

		void initializeVulkan();
		void enableVulkanValidationLayers();
		void initializeDevice();
		void initializeAllocator();
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