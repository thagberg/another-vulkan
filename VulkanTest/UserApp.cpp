#include "UserApp.h"
#include "vulkanapp.h"
#include "InputManager.h"
#include "imgui\imgui.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

GLFWwindow* initializeWindow(int width, int height, const char* windowTitle) {
    int glfwStatus = glfwInit();
    if (glfwStatus != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(width, height, windowTitle, nullptr, nullptr);
    return window;
}


std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back("VK_EXT_debug_utils");

    return extensions;
}

UserApp::UserApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
    mWindowWidth(windowWidth),
    mWindowHeight(windowHeight),
    mWindowTitle(windowTitle),
    mVulkanInstance(VK_NULL_HANDLE),
    mWindowSurface(VK_NULL_HANDLE),
	mApp(std::make_unique<hvk::VulkanApp>()),
    mWindow(initializeWindow(mWindowWidth, mWindowHeight, mWindowTitle), glfwDestroyWindow),
    mClock()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    std::vector<const char*> extensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &mVulkanInstance);

    if (instanceResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to initialize Vulkan");
    }

    if (glfwCreateWindowSurface(mVulkanInstance, mWindow.get(), nullptr, &mWindowSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Window Surface");
    }

    glfwSetWindowUserPointer(mWindow.get(), this);
    glfwSetFramebufferSizeCallback(mWindow.get(), UserApp::handleWindowResize);
	glfwSetCharCallback(mWindow.get(), UserApp::handleCharInput);
    ImGui::CreateContext();
	// must init InputManager after we've created an ImGui context
    hvk::InputManager::init(mWindow);

	mApp->init(mWindowWidth, mWindowHeight, mVulkanInstance, mWindowSurface);
}

UserApp::~UserApp()
{
    glfwTerminate();
    mApp.reset();
    vkDestroySurfaceKHR(mVulkanInstance, mWindowSurface, nullptr);
    vkDestroyInstance(mVulkanInstance, nullptr);
}

void UserApp::handleWindowResize(GLFWwindow* window, int width, int height) 
{
    UserApp* thisApp = reinterpret_cast<UserApp*>(glfwGetWindowUserPointer(window));
    thisApp->mWindowWidth = width;
    thisApp->mWindowHeight = height;
    thisApp->mApp->recreateSwapchain(thisApp->mWindowWidth, thisApp->mWindowHeight);
}

void UserApp::handleCharInput(GLFWwindow* window, uint32_t character)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter(character);
}

void UserApp::runApp()
{
    double frameTime = 0.f;
    bool shouldClose = false;
    while (!shouldClose)
    {
        mClock.start();
		frameTime = mClock.getDelta();

        shouldClose |= glfwWindowShouldClose(mWindow.get());

        shouldClose |= run(frameTime);
        shouldClose |= mApp->update(frameTime);
        mClock.end();
    }
}

void UserApp::doClose()
{
	close();
}

void UserApp::toggleCursor(bool enabled)
{
    int toggle = enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
    glfwSetInputMode(mWindow.get(), GLFW_CURSOR, toggle);
}


void UserApp::addStaticMeshInstance(hvk::HVK_shared<hvk::StaticMeshRenderObject> node)
{
    mApp->addStaticMeshInstance(node);
}

void UserApp::addDynamicLight(hvk::HVK_shared<hvk::Light> light)
{
    mApp->addDynamicLight(light);
}

void UserApp::addDebugMeshInstance(hvk::HVK_shared<hvk::DebugMeshRenderObject> node)
{
    mApp->addDebugMeshInstance(node);
}

void UserApp::activateCamera(hvk::HVK_shared<hvk::Camera> camera)
{
    mApp->setActiveCamera(camera);
}

void UserApp::setGammaCorrection(float gamma)
{
    mApp->setGammaCorrection(gamma);
}

void UserApp::setUseSRGBTex(bool useSRGBTex)
{
    mApp->setUseSRGBTex(useSRGBTex);
}

float UserApp::getGammaCorrection() 
{ 
    return mApp->getGammaCorrection(); 
}

bool UserApp::isUseSRGBTex() 
{ 
    return mApp->isUseSRGBTex(); 
}

VkDevice UserApp::getDevice()
{
    return mApp->getDevice();
}

hvk::HVK_shared<hvk::AmbientLight> UserApp::getAmbientLight()
{
	return mApp->getAmbientLight();
}

hvk::HVK_shared<hvk::GammaSettings> UserApp::getGammaSettings()
{
	return mApp->getGammaSettings();
}

hvk::HVK_shared<hvk::PBRWeight> UserApp::getPBRWeight()
{
	return mApp->getPBRWeight();
}

hvk::HVK_shared<hvk::ExposureSettings> UserApp::getExposureSettings()
{
	return mApp->getExposureSettings();
}

hvk::HVK_shared<hvk::SkySettings> UserApp::getSkySettings()
{
	return mApp->getSkySettings();
}

void UserApp::generateEnvironmentMap()
{
	mApp->generateEnvironmentMap();
}

void UserApp::useEnvironmentMap()
{
	mApp->useEnvironmentMap();
}

void UserApp::useIrradianceMap()
{
	mApp->useIrradianceMap();
}

void UserApp::usePrefilteredMap(float lod)
{
	mApp->usePrefilteredMap(lod);
}

hvk::ModelPipeline& UserApp::getModelPipeline()
{
    return mApp->getModelPipeline();
}

std::shared_ptr<hvk::StaticMeshGenerator> UserApp::getMeshRenderer()
{
	return mApp->getMeshRenderer();
}
