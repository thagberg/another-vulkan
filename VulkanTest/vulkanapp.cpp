#include "stdafx.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkanapp.h"
#include "vulkan-util.h"
#include "RenderObject.h"

hvk::VulkanApp* currentApp;

enum CameraControl {
	move_left,
	move_right,
	move_up,
	move_down,
	move_forward,
	move_backward
};

std::unordered_map<int, CameraControl> cameraControlMapping({
	{GLFW_KEY_A, CameraControl::move_left},
	{GLFW_KEY_D, CameraControl::move_right},
	{GLFW_KEY_W, CameraControl::move_forward},
	{GLFW_KEY_S, CameraControl::move_backward},
	{GLFW_KEY_Q, CameraControl::move_up},
	{GLFW_KEY_E, CameraControl::move_down}
});

std::unordered_map<CameraControl, bool> cameraControls({
	{CameraControl::move_left, false},
	{CameraControl::move_right, false},
	{CameraControl::move_up, false},
	{CameraControl::move_down, false},
	{CameraControl::move_forward, false},
	{CameraControl::move_backward, false}
});

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<hvk::Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    std::cout << "Key " << ((action == GLFW_PRESS || action == GLFW_REPEAT) ? "Pressed" : "Released") << ": " << key << std::endl;
    currentApp->processKeyInput(key, (action == GLFW_PRESS || action == GLFW_REPEAT));
}

namespace hvk {

    VulkanApp::VulkanApp(int width, int height, const char* windowTitle) :
        mWindowWidth(width),
        mWindowHeight(height),
        mInstance(VK_NULL_HANDLE),
        mPhysicalDevice(VK_NULL_HANDLE),
        mDevice(VK_NULL_HANDLE),
        //mPipelineLayout(VK_NULL_HANDLE),
        //mGraphicsPipeline(VK_NULL_HANDLE),
        mWindow(hvk::initializeWindow(width, height, windowTitle), glfwDestroyWindow),
		mRenderer()
    {
        // TODO: this is super bad, but IDGAF right now
        currentApp = this;
    }

    VulkanApp::~VulkanApp() {
        glfwTerminate();
        vkDestroySemaphore(mDevice, mImageAvailable, nullptr);
        vkDestroySemaphore(mDevice, mRenderFinished, nullptr);
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        //vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
        //vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
        for (auto framebuffer : mFramebuffers) {
            vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
        }
        for (auto imageView : mImageViews) {
            vkDestroyImageView(mDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(mDevice, mSwapchain.swapchain, nullptr);
	
		// TODO: destroy Renderer

        //vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        vkDestroyDevice(mDevice, nullptr);
        vkDestroyInstance(mInstance, nullptr);
    }

    void VulkanApp::initializeVulkan() {
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

        VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &mInstance);

        if (instanceResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to initialize Vulkan");
        }
    }

    void VulkanApp::enableVulkanValidationLayers() {
        VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = debugCallback;
        debugInfo.pUserData = nullptr;

        VkDebugUtilsMessengerEXT debugMessenger;
        VkResult debugResult = CreateDebugUtilsMessengerEXT(mInstance, &debugInfo, &debugMessenger);

        if (debugResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to enable Vulkan Validation Layers");
        }
    }

    void VulkanApp::initializeDevice() {
        // Cheating... just take the first physical device found
        mPhysicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;     // enable anisotropic filtering
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());
        mPhysicalDevice = devices[0];

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(mPhysicalDevice, &supportedFeatures);
        if (!supportedFeatures.samplerAnisotropy) {
            throw std::runtime_error("Physical Device does not support Anisotropic Filtering");
        }

        uint32_t queueFamilyCount = 0;
        mGraphicsIndex = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueFamilies.data());
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                break;
            }
        }

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = mGraphicsIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pEnabledFeatures = &deviceFeatures;
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
        //deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        //deviceInfo.ppEnabledLayerNames = validationLayers.data();

        VkResult deviceStatus = vkCreateDevice(mPhysicalDevice, &deviceInfo, nullptr, &mDevice);
        if (deviceStatus != VK_SUCCESS) {
            throw std::runtime_error("Failed to initialize Device");
        }
    }

    void VulkanApp::initializeRenderer() {
        vkGetDeviceQueue(mDevice, mGraphicsIndex, 0, &mGraphicsQueue);

        if (glfwCreateWindowSurface(mInstance, mWindow.get(), nullptr, &mSurface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Window Surface");
        }

        VkBool32 surfaceSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, mGraphicsIndex, mSurface, &surfaceSupported);
        if (!surfaceSupported) {
            throw std::runtime_error("Surface not supported by Physical Device");
        }

        if (createSwapchain(mPhysicalDevice, mDevice, mSurface, mWindowWidth, mWindowHeight, mSwapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Swapchain");
        }

        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, nullptr);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, nullptr);
        mSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, mSwapchainImages.data());

        mImageViews.resize(mSwapchainImages.size());
        for (size_t i = 0; i < mSwapchainImages.size(); i++) {
            mImageViews[i] = createImageView(mDevice, mSwapchainImages[i], mSwapchain.swapchainImageFormat);
        }


        mRenderPass = createRenderPass(mDevice, mSwapchain.swapchainImageFormat);

        createFramebuffers(mDevice, mImageViews, mRenderPass, mSwapchain.swapchainExtent, mFramebuffers);

        mCommandPool = createCommandPool(mDevice, mGraphicsIndex);

		// create Camera object
		mCameraNode = std::make_shared<Camera>(
			45.0f,
			mSwapchain.swapchainExtent.width / (float)mSwapchain.swapchainExtent.height,
			0.1f,
			10.0f,
			nullptr,
			glm::mat4(1.0f));
		QueueFamilies families = {
			mGraphicsIndex,
			mGraphicsIndex
		};
		VulkanDevice device = {
			mPhysicalDevice,
			mDevice,
			families
		};
		mRenderer.init(device, mGraphicsQueue, mRenderPass, mCameraNode, mSwapchain.swapchainImageFormat, mSwapchain.swapchainExtent);

		RenderObjRef newObj = std::make_shared<RenderObject>(nullptr, glm::mat4(1.0f));
		glm::mat4 obj2Trans = glm::mat4(1.0f);
		obj2Trans = glm::translate(obj2Trans, glm::vec3(0.3f, 0.2f, 1.0f));
		RenderObjRef obj2 = std::make_shared<RenderObject>(nullptr, obj2Trans);
		mRenderer.addRenderable(obj2);
		mRenderer.addRenderable(newObj);

        mObjectNode = std::make_shared<Node>(nullptr, glm::mat4(1.0f));


        mImageAvailable = createSemaphore(mDevice);
    }

    void VulkanApp::init() {
        try {
            std::cout << "init vulkan" << std::endl;
            initializeVulkan();
            enableVulkanValidationLayers();
            std::cout << "init device" << std::endl;
            initializeDevice();
            std::cout << "init renderer" << std::endl;
            initializeRenderer();
            std::cout << "done initializing" << std::endl;
        }
        catch (const std::runtime_error& error) {
            std::cout << "Error during initialization: " << error.what() << std::endl;
        }
    }

    void VulkanApp::drawFrame() {
        uint32_t imageIndex;
        vkAcquireNextImageKHR(
            mDevice,
            mSwapchain.swapchain,
            std::numeric_limits<uint64_t>::max(),
            mImageAvailable,
            VK_NULL_HANDLE,
            &imageIndex);

		mRenderFinished = mRenderer.drawFrame(mFramebuffers[imageIndex], &mImageAvailable, 1);

        VkSwapchainKHR swapchains[] = { mSwapchain.swapchain };
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mRenderFinished;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(mGraphicsQueue, &presentInfo);
    }

    void VulkanApp::run() {
        glfwSetKeyCallback(mWindow.get(), keyCallback);
        while (!glfwWindowShouldClose(mWindow.get())) {
            glfwPollEvents();

			// camera updates
			if (cameraControls[CameraControl::move_left]) {
				mCameraNode->setLocalTransform(
					glm::translate(mCameraNode->getLocalTransform(), glm::vec3(0.01f, 0.f, 0.f)));
			}
			if (cameraControls[CameraControl::move_right]) {
				mCameraNode->setLocalTransform(
					glm::translate(mCameraNode->getLocalTransform(), glm::vec3(-0.01f, 0.f, 0.f)));
			}
			if (cameraControls[CameraControl::move_forward]) {
				mCameraNode->setLocalTransform(
					glm::translate(mCameraNode->getLocalTransform(), glm::vec3(0.f, 0.f, 0.01f)));
			}
			if (cameraControls[CameraControl::move_backward]) {
				mCameraNode->setLocalTransform(
					glm::translate(mCameraNode->getLocalTransform(), glm::vec3(0.f, 0.f, -0.01f)));
			}
			if (cameraControls[CameraControl::move_up]) {
				mCameraNode->setLocalTransform(
					glm::translate(mCameraNode->getLocalTransform(), glm::vec3(0.f, 0.01f, 0.f)));
			}
			if (cameraControls[CameraControl::move_down]) {
				mCameraNode->setLocalTransform(
					glm::translate(mCameraNode->getLocalTransform(), glm::vec3(0.f, -0.01f, 0.f)));
			}

            drawFrame();
        }

        vkDeviceWaitIdle(mDevice);
    }

    void VulkanApp::processKeyInput(int keyCode, bool pressed) {
        if (pressed) {
            if (keyCode == GLFW_KEY_ESCAPE) {
                glfwSetWindowShouldClose(mWindow.get(), GLFW_TRUE);
            }
            else if (keyCode == GLFW_KEY_LEFT) {
                mObjectNode->setLocalTransform(glm::translate(mObjectNode->getLocalTransform(), glm::vec3(-0.1f, 0.0f, 0.0f)));
            }
            else if (keyCode == GLFW_KEY_RIGHT) {
                mObjectNode->setLocalTransform(glm::translate(mObjectNode->getLocalTransform(), glm::vec3(0.1f, 0.0f, 0.0f)));
            }
            else if (keyCode == GLFW_KEY_UP) {
                mObjectNode->setLocalTransform(glm::translate(mObjectNode->getLocalTransform(), glm::vec3(0.0f, 0.0f, 0.1f)));
            }
            else if (keyCode == GLFW_KEY_DOWN) {
                mObjectNode->setLocalTransform(glm::translate(mObjectNode->getLocalTransform(), glm::vec3(0.0f, 0.0f, -0.1f)));
            }
        }

		auto mapping = cameraControlMapping.find(keyCode);
		if (mapping != cameraControlMapping.end()) {
			cameraControls[mapping->second] = pressed;
		}
    }
}