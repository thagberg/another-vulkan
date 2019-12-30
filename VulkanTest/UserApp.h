#pragma once

#include <memory>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "Clock.h"
#include "HvkUtil.h"
#include "CubemapGenerator.h"
/*
#include "RenderObject.h"
#include "Light.h"
#include "Camera.h"
*/

namespace hvk
{
	class VulkanApp;
	class StaticMeshRenderObject;
	class Light;
	class DebugMeshRenderObject;
	class Camera;
    class ModelPipeline;
	class StaticMeshGenerator;
	class UiDrawGenerator;
	class DebugDrawGenerator;
	class QuadGenerator;
	struct AmbientLight;
	struct GammaSettings;
	struct PBRWeight;
	struct ExposureSettings;
	struct SkySettings;
	struct Swapchain;
}

namespace hvk
{
	class UserApp
	{
	private:
		uint32_t mWindowWidth;
		uint32_t mWindowHeight;
		const char* mWindowTitle;
		VkInstance mVulkanInstance;
		VkSurfaceKHR mWindowSurface;
		std::unique_ptr<hvk::VulkanApp> mApp;
		std::shared_ptr<GLFWwindow> mWindow;
		hvk::Clock mClock;

		// Moving things out of vulkanapp
		VkRenderPass mPBRRenderPass;
		VkRenderPass mFinalRenderPass;
		Swapchain mSwapchain;
		std::vector<VkImage> mSwapchainImages;
		std::vector<VkImageView> mSwapchainViews;
		std::vector<VkFramebuffer> mSwapFramebuffers;
		VkFramebuffer mPBRFramebuffer;
		std::shared_ptr<TextureMap> mPBRPassMap;
		RuntimeResource<VkImage> mPBRDepthImage;
		VkImageView mPBRDepthView;
		std::shared_ptr<StaticMeshGenerator> mPBRMeshRenderer;
		std::shared_ptr<UiDrawGenerator> mUiRenderer;
		std::shared_ptr<DebugDrawGenerator> mDebugRenderer;
		std::shared_ptr<CubemapGenerator<SkySettings>> mSkyboxRenderer;
		std::shared_ptr<QuadGenerator> mQuadRenderer;
		std::shared_ptr<AmbientLight> mAmbientLight;
		std::shared_ptr<TextureMap> mEnvironmentMap;
		std::shared_ptr<TextureMap> mIrradianceMap;
		std::shared_ptr<TextureMap> mPrefilteredMap;
		std::shared_ptr<TextureMap> mBrdfLutMap;
		GammaSettings mGammaSettings;
		PBRWeight mPBRWeight;
		ExposureSettings mExposureSettings;
		SkySettings mSkySettings;

		void createPBRRenderPass();
		void createPBRFramebuffers();
		void createFinalRenderPass();
		void createSwapFramebuffers();
        void drawFrame(double frametime);

		static void handleWindowResize(GLFWwindow* window, int width, int height);
		static void handleCharInput(GLFWwindow* window, uint32_t character);

	protected:
		virtual bool run(double frameTime) = 0;
		virtual void close() = 0;

	public:
		UserApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle);
		virtual ~UserApp();

		void runApp();
		void doClose();
		void toggleCursor(bool enabled);
		void addStaticMeshInstance(hvk::HVK_shared<hvk::StaticMeshRenderObject> node);
		void addDynamicLight(hvk::HVK_shared<hvk::Light> light);
		void addDebugMeshInstance(hvk::HVK_shared<hvk::DebugMeshRenderObject> node);
		void activateCamera(hvk::HVK_shared<hvk::Camera> camera);
		void setGammaCorrection(float gamma);
		//hvk::HVK_shared<hvk::GammaSettings> getGammaSettings();
		//hvk::HVK_shared<hvk::PBRWeight> getPBRWeight();
		//hvk::HVK_shared<hvk::ExposureSettings> getExposureSettings();
		//hvk::HVK_shared<hvk::SkySettings> getSkySettings();
		//void setUseSRGBTex(bool useSRGBTex);
		//hvk::HVK_shared<hvk::AmbientLight> getAmbientLight();
		//float getGammaCorrection();
		//bool isUseSRGBTex();
		//VkDevice getDevice();
		//void generateEnvironmentMap();
		//void useEnvironmentMap();
		//void useIrradianceMap();
		//void usePrefilteredMap(float lod);
		//hvk::ModelPipeline& getModelPipeline();
		//std::shared_ptr<hvk::StaticMeshGenerator> getMeshRenderer();
	};
}
