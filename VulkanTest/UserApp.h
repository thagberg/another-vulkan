#pragma once

#include <memory>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "Clock.h"
#include "HvkUtil.h"
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
	struct AmbientLight;
	struct GammaSettings;
	struct PBRWeight;
	struct ExposureSettings;
	struct SkySettings;
}

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
	hvk::HVK_shared<hvk::GammaSettings> getGammaSettings();
	hvk::HVK_shared<hvk::PBRWeight> getPBRWeight();
	hvk::HVK_shared<hvk::ExposureSettings> getExposureSettings();
	hvk::HVK_shared<hvk::SkySettings> getSkySettings();
    void setUseSRGBTex(bool useSRGBTex);
	hvk::HVK_shared<hvk::AmbientLight> getAmbientLight();
    float getGammaCorrection();
    bool isUseSRGBTex();
    VkDevice getDevice();
	void generateEnvironmentMap();
	void useEnvironmentMap();
	void useIrradianceMap();
	void usePrefilteredMap(float lod);
};
