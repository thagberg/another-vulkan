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

		std::shared_ptr<StaticMeshGenerator> mMeshRenderer;	// M
		std::shared_ptr<UiDrawGenerator> mUiRenderer;	// M
		std::shared_ptr<DebugDrawGenerator> mDebugRenderer;	// M
		HVK_shared<CubemapGenerator<SkySettings>> mSkyboxRenderer; // M
        HVK_shared<QuadGenerator> mQuadRenderer; // M
		HVK_shared<AmbientLight> mAmbientLight;	// M

		VkFence mRenderFence;

		HVK_shared<GammaSettings> mGammaSettings; // M
		HVK_shared<PBRWeight> mPBRWeight; // M
		HVK_shared<ExposureSettings> mExposureSettings; // M
		HVK_shared<SkySettings> mSkySettings; // M

		HVK_shared<TextureMap> mEnvironmentMap; // M
		HVK_shared<TextureMap> mIrradianceMap; // M
		HVK_shared<TextureMap> mPrefilteredMap; // M
		HVK_shared<TextureMap> mBdrfLutMap; // M

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
		HVK_shared<ExposureSettings> getExposureSettings() { return mExposureSettings; }
		HVK_shared<SkySettings> getSkySettings() { return mSkySettings; }
        void setUseSRGBTex(bool useSRGBTex);
        float getGammaCorrection() { return mMeshRenderer->getGammaCorrection(); }
        bool isUseSRGBTex() { return mMeshRenderer->isUseSRGBTex(); }
        VkDevice getDevice() { return mDevice; }
		void generateEnvironmentMap(
			std::shared_ptr<TextureMap> environmentMap,
			std::shared_ptr<TextureMap> irradianceMap,
			std::shared_ptr<TextureMap> prefilteredMap,
			std::shared_ptr<TextureMap> brdfLutMap);
		void useEnvironmentMap() { mSkyboxRenderer->setCubemap(mEnvironmentMap); }
		void useIrradianceMap() { mSkyboxRenderer->setCubemap(mIrradianceMap); }
		void usePrefilteredMap(float lod) 
		{ 
			//mSkySettings->lod = lod;
			mSkyboxRenderer->setCubemap(mPrefilteredMap); 
		}

		// ugly stuff... get rid of this ASAP
		std::shared_ptr<StaticMeshGenerator> getMeshRenderer()
		{
			return mMeshRenderer;
		}

		// new render paradigm
		uint32_t renderPrepare(VkSwapchainKHR& swapchain);
		VkCommandBufferInheritanceInfo renderpassBegin(const VkRenderPassBeginInfo& renderBegin);
		void renderpassExecuteAndClose(std::vector<VkCommandBuffer>& secondaryBuffers);
		void renderFinish();
		void renderSubmit();
		void renderPresent(uint32_t swapIndex);
	};
}