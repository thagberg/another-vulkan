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
#include "SkyboxGenerator.h"
#include "QuadGenerator.h"
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
		VkRenderPass mColorRenderPass;
        VkRenderPass mFinalRenderPass;
		VkCommandPool mCommandPool;
		VkCommandBuffer mPrimaryCommandBuffer;

		SwapchainImageViews mFinalPassImageViews;
		SwapchainImages mFinalPassSwapchainImages;
        HVK_shared<TextureMap> mColorPassMap;
		Resource<VkImage> mDepthResource;
		VkImageView mDepthView;
		FrameBuffers mFinalPassFramebuffers;
        VkFramebuffer mColorPassFramebuffer;
		hvk::Swapchain mSwapchain;

		VkSemaphore mImageAvailable;
		VkSemaphore mRenderFinished;
		VkSemaphore mFinalRenderFinished;

		VmaAllocator mAllocator;

		CameraRef mActiveCamera;

		std::shared_ptr<StaticMeshGenerator> mMeshRenderer;
		std::shared_ptr<UiDrawGenerator> mUiRenderer;
		std::shared_ptr<DebugDrawGenerator> mDebugRenderer;
		HVK_shared<SkyboxGenerator> mSkyboxRenderer;
        HVK_shared<QuadGenerator> mQuadRenderer;
		HVK_shared<AmbientLight> mAmbientLight;

		VkFence mRenderFence;

		std::vector<VkCommandBuffer> mSecondaryCommandBuffers;

		HVK_shared<GammaSettings> mGammaSettings;
		HVK_shared<PBRWeight> mPBRWeight;

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
		void setAmbientLight(HVK_shared<AmbientLight> ambientLight);
		HVK_shared<AmbientLight> getAmbientLight() { return mAmbientLight; }
        void setActiveCamera(HVK_shared<Camera> camera);
		void recreateSwapchain(uint32_t surfaceWidth, uint32_t surfaceHeight);

        void setGammaCorrection(float gamma);
		HVK_shared<GammaSettings> getGammaSettings() { return mGammaSettings; }
		HVK_shared<PBRWeight> getPBRWeight() { return mPBRWeight; }
        void setUseSRGBTex(bool useSRGBTex);
        float getGammaCorrection() { return mMeshRenderer->getGammaCorrection(); }
        bool isUseSRGBTex() { return mMeshRenderer->isUseSRGBTex(); }
        VkDevice getDevice() { return mDevice; }
	};
}