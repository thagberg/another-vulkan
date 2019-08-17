#include "stdafx.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_SWIZZLE_XYZW
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "tiny_gltf.h"

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
    currentApp->processKeyInput(key, (action == GLFW_PRESS || action == GLFW_REPEAT));
}

void mouseCallback(GLFWwindow* window, double x, double y) {
	currentApp->processMouseInput(x, y);
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
		mRenderer(),
		mLastX(0.f),
		mLastY(0.f)
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

        mCommandPool = createCommandPool(mDevice, mGraphicsIndex);

		// create allocator
		VmaAllocatorCreateInfo allocatorCreate = {};
		allocatorCreate.physicalDevice = mPhysicalDevice;
		allocatorCreate.device = mDevice;
		vmaCreateAllocator(&allocatorCreate, &mAllocator);

		// create depth image and view
		VkImageCreateInfo depthImageCreate = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		depthImageCreate.imageType = VK_IMAGE_TYPE_2D;
		depthImageCreate.extent.width = mSwapchain.swapchainExtent.width;
		depthImageCreate.extent.height = mSwapchain.swapchainExtent.height;
		depthImageCreate.extent.depth = 1;
		depthImageCreate.mipLevels = 1;
		depthImageCreate.arrayLayers = 1;
		depthImageCreate.format = VK_FORMAT_D32_SFLOAT;
		depthImageCreate.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthImageCreate.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthImageCreate.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthImageCreate.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		depthImageCreate.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo depthImageAllocationCreate = {};
        depthImageAllocationCreate.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(
            mAllocator,
            &depthImageCreate,
            &depthImageAllocationCreate,
            &mDepthResource.memoryResource,
            &mDepthResource.allocation,
            &mDepthResource.allocationInfo);

		mDepthView = createImageView(mDevice, mDepthResource.memoryResource, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
		transitionImageLayout(
			mDevice, 
			mCommandPool, 
			mGraphicsQueue, 
			mDepthResource.memoryResource, 
			VK_FORMAT_D32_SFLOAT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


        mRenderPass = createRenderPass(mDevice, mSwapchain.swapchainImageFormat);

        createFramebuffers(mDevice, mImageViews, mDepthView, mRenderPass, mSwapchain.swapchainExtent, mFramebuffers);

		// create Camera object
		mCameraNode = std::make_shared<Camera>(
			45.0f,
			mSwapchain.swapchainExtent.width / (float)mSwapchain.swapchainExtent.height,
			0.1f,
			1000.0f,
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
		mRenderer.init(device, mAllocator, mGraphicsQueue, mRenderPass, mCameraNode, mSwapchain.swapchainImageFormat, mSwapchain.swapchainExtent);

		/*
		RenderObjRef newObj = std::make_shared<RenderObject>(
            nullptr, 
            glm::mat4(1.0f), 
            std::make_shared<std::vector<Vertex>>(vertices),
            std::make_shared<std::vector<uint16_t>>(indices));
		glm::mat4 obj2Trans = glm::mat4(1.0f);
		obj2Trans = glm::translate(obj2Trans, glm::vec3(0.3f, 0.2f, -5.0f));
		glm::mat4 obj3Trans  = glm::rotate(glm::mat4(1.0f), 0.1f, glm::vec3(0.f, 1.f, 0.f));
		obj3Trans = glm::translate(obj3Trans, glm::vec3(1.f, -4.f, 1.f));
		RenderObjRef obj2 = std::make_shared<RenderObject>(
            nullptr, 
            obj2Trans,
            std::make_shared<std::vector<Vertex>>(vertices),
            std::make_shared<std::vector<uint16_t>>(indices));
		RenderObjRef obj3 = std::make_shared<RenderObject>(
            nullptr, 
            obj3Trans,
            std::make_shared<std::vector<Vertex>>(vertices),
            std::make_shared<std::vector<uint16_t>>(indices));
		mRenderer.addRenderable(obj2);
		mRenderer.addRenderable(newObj);
		mRenderer.addRenderable(obj3);
		*/

        //bool modelLoaded = modelLoader.LoadASCIIFromFile(&model, &err, &warn, "resources/duck/Duck.gltf");
		glm::mat4 modelTransform = glm::mat4(1.0f);
		modelTransform = glm::scale(modelTransform, glm::vec3(0.01f, 0.01f, 0.01f));
		RenderObjRef modelObj = RenderObject::createFromGltf("resources/duck/Duck.gltf", nullptr, modelTransform);
		mRenderer.addRenderable(modelObj);



		//glm::lookAt(mCameraNode->getWorldPosition(), )
		//mCameraNode->setLocalTransform(mCameraNode->getLocalTransform() * glm::lookAt());
		//mCameraNode->lookAt()

		//mCameraNode->setLocalPosition(glm::vec3(0.f, 2.f, 2.f));

		//std::cout << "Obj 1 position: " << glm::to_string(newObj->getWorldPosition()) << std::endl;
		//std::cout << "Obj 2 position: " << glm::to_string(obj2->getWorldPosition()) << std::endl;
		std::cout << "Camera position: " << glm::to_string(mCameraNode->getWorldPosition()) << std::endl;
		std::cout << "Camera Up Vector: " << glm::to_string(mCameraNode->getUpVector()) << std::endl;
		std::cout << "Camera Forward Vector: " << glm::to_string(mCameraNode->getForwardVector()) << std::endl;
		std::cout << "Camera Right Vector: " << glm::to_string(mCameraNode->getRightVector()) << std::endl;

        mObjectNode = std::make_shared<Node>(nullptr, glm::mat4(1.0f));


        mImageAvailable = createSemaphore(mDevice);
    }

	void VulkanApp::initializeApp() {
		glfwSetInputMode(mWindow.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
            std::cout << "init application" << std::endl;
			initializeApp();
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
		glfwGetCursorPos(mWindow.get(), &mLastX, &mLastY);
		glfwSetCursorPosCallback(mWindow.get(), mouseCallback);
        while (!glfwWindowShouldClose(mWindow.get())) {
            glfwPollEvents();

			// camera updates
			glm::vec3 forwardMovement = 0.01f * mCameraNode->getForwardVector();
			glm::vec3 lateralMovement = 0.01f * mCameraNode->getRightVector();
			glm::vec3 verticalMovement = 0.01f * mCameraNode->getUpVector();
			if (cameraControls[CameraControl::move_left]) {
				mCameraNode->translateLocal(-1.0f * lateralMovement);
			}
			if (cameraControls[CameraControl::move_right]) {
				mCameraNode->translateLocal(lateralMovement);
			}
			if (cameraControls[CameraControl::move_forward]) {
				mCameraNode->translateLocal(-1.0f * forwardMovement);
			}
			if (cameraControls[CameraControl::move_backward]) {
				mCameraNode->translateLocal(forwardMovement);
			}
			if (cameraControls[CameraControl::move_up]) {
				mCameraNode->translateLocal(verticalMovement);
			}
			if (cameraControls[CameraControl::move_down]) {
				mCameraNode->translateLocal(-1.0f * verticalMovement);
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
			else if (keyCode == GLFW_KEY_Y) {
				Renderer::setDrawNormals(!Renderer::getDrawNormals());
			}
        }

		auto mapping = cameraControlMapping.find(keyCode);
		if (mapping != cameraControlMapping.end()) {
			cameraControls[mapping->second] = pressed;
		}
    }

	void VulkanApp::processMouseInput(double x, double y) {
		float sensitivity = 0.1f;
		double deltaX = mLastX - x;
		double deltaY = y - mLastY;
		mLastX = x;
		mLastY = y;

		float pitch = deltaY * sensitivity;
		float yaw = deltaX * sensitivity;

		mCameraNode->rotate(glm::radians(pitch), glm::radians(yaw));
	}
}