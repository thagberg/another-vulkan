#include <vector>
#include <iostream>
#include <limits>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "vulkanapp.h"
#include "vulkan-util.h"
#include "RenderObject.h"
#include "InputManager.h"

#include "HvkUtil.h"

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


namespace hvk {

    VulkanApp::VulkanApp() :
        mSurfaceWidth(),
        mSurfaceHeight(),
        mInstance(VK_NULL_HANDLE),
        mSurface(VK_NULL_HANDLE),
        mDevice(VK_NULL_HANDLE),
        mPhysicalDevice(VK_NULL_HANDLE),
        mGraphicsIndex(),
        mRenderPass(VK_NULL_HANDLE),
        mCommandPool(VK_NULL_HANDLE),
        mPrimaryCommandBuffer(VK_NULL_HANDLE),
        mImageViews(),
        mSwapchainImages(),
        mDepthResource(),
        mFramebuffers(),
        mSwapchain(),
        mImageAvailable(VK_NULL_HANDLE),
        mRenderFinished(VK_NULL_HANDLE),
        mAllocator(),
        mActiveCamera(nullptr),
        mMeshRenderer(nullptr),
        mUiRenderer(nullptr),
        mDebugRenderer(nullptr),
		mSkyboxRenderer(nullptr),
        mRenderFence(VK_NULL_HANDLE)
    {

    }

    VulkanApp::~VulkanApp() {
        vkDeviceWaitIdle(mDevice);
        vkDestroySemaphore(mDevice, mImageAvailable, nullptr);
        vkDestroySemaphore(mDevice, mRenderFinished, nullptr);
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

		cleanupSwapchain();

        mMeshRenderer.reset();
        mUiRenderer.reset();
        mDebugRenderer.reset();
		mSkyboxRenderer.reset();

        vkDestroySemaphore(mDevice, mImageAvailable, nullptr);
        vkDestroySemaphore(mDevice, mRenderFinished, nullptr);
        vkDestroyFence(mDevice, mRenderFence, nullptr);

        vmaDestroyAllocator(mAllocator);

        vkDestroyDevice(mDevice, nullptr);
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
		// create allocator
		VmaAllocatorCreateInfo allocatorCreate = {};
		allocatorCreate.physicalDevice = mPhysicalDevice;
		allocatorCreate.device = mDevice;
		vmaCreateAllocator(&allocatorCreate, &mAllocator);

        vkGetDeviceQueue(mDevice, mGraphicsIndex, 0, &mGraphicsQueue);
        mCommandPool = createCommandPool(mDevice, mGraphicsIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        VkBool32 surfaceSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, mGraphicsIndex, mSurface, &surfaceSupported);
        if (!surfaceSupported) {
            throw std::runtime_error("Surface not supported by Physical Device");
        }

        if (createSwapchain(mPhysicalDevice, mDevice, mSurface, mSurfaceWidth, mSurfaceHeight, mSwapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Swapchain");
        }

		initFramebuffers();

		QueueFamilies families = {
			mGraphicsIndex,
			mGraphicsIndex
		};
		VulkanDevice device = {
			mPhysicalDevice,
			mDevice,
			families
		};

		// Create fence for rendering complete
		VkFenceCreateInfo fenceCreate = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreate.pNext = nullptr;
		fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		mRenderFinished = createSemaphore(mDevice);
		assert(vkCreateFence(device.device, &fenceCreate, nullptr, &mRenderFence) == VK_SUCCESS);

		// Allocate command buffer
		VkCommandBufferAllocateInfo bufferAlloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		bufferAlloc.commandBufferCount = 1;
		bufferAlloc.commandPool = mCommandPool;
		bufferAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		assert(vkAllocateCommandBuffers(mDevice, &bufferAlloc, &mPrimaryCommandBuffer) == VK_SUCCESS);

		mMeshRenderer = std::make_shared<StaticMeshGenerator>(
            device, 
            mAllocator, 
            mGraphicsQueue, 
            mRenderPass, 
            mCommandPool);

		mUiRenderer = std::make_shared<UiDrawGenerator>(
            device, 
            mAllocator, 
            mGraphicsQueue, 
            mRenderPass, 
            mCommandPool, 
            mSwapchain.swapchainExtent);

		mDebugRenderer = std::make_shared<DebugDrawGenerator>(
            device, 
            mAllocator, 
            mGraphicsQueue, 
            mRenderPass, 
            mCommandPool);

		mSkyboxRenderer = HVK_make_shared<SkyboxGenerator>(
			device,
			mAllocator,
			mGraphicsQueue,
			mRenderPass,
			mCommandPool);

        mImageAvailable = createSemaphore(mDevice);
    }

    void VulkanApp::addStaticMeshInstance(HVK_shared<StaticMeshRenderObject> node)
    {
        mMeshRenderer->addStaticMeshObject(node);
    }

    void VulkanApp::addDynamicLight(HVK_shared<Light> light)
    {
        mMeshRenderer->addLight(light);
    }

    void VulkanApp::addDebugMeshInstance(HVK_shared<DebugMeshRenderObject> node)
    {
        mDebugRenderer->addDebugMeshObject(node);
    }

	void VulkanApp::recreateSwapchain(uint32_t surfaceWidth, uint32_t surfaceHeight) {
		vkDeviceWaitIdle(mDevice);

        mSurfaceWidth = surfaceWidth;
        mSurfaceHeight = surfaceHeight;

		mMeshRenderer->invalidate();
		mUiRenderer->invalidate();
		mDebugRenderer->invalidate();
		cleanupSwapchain();

        if (createSwapchain(mPhysicalDevice, mDevice, mSurface, mSurfaceWidth, mSurfaceHeight, mSwapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Swapchain");
        }

		initFramebuffers();
        if (mActiveCamera)
        {
            mActiveCamera->updateProjection(
                90.0f,
                mSwapchain.swapchainExtent.width / (float)mSwapchain.swapchainExtent.height,
                0.001f,
                1000.0f);
        }
		mMeshRenderer->updateRenderPass(mRenderPass);
		mUiRenderer->updateRenderPass(mRenderPass, mSwapchain.swapchainExtent);
		mDebugRenderer->updateRenderPass(mRenderPass);
	}

    void VulkanApp::init(
            uint32_t surfaceWidth, 
            uint32_t surfaceHeight,
            VkInstance vulkanInstance,
            VkSurfaceKHR surface)
    {
        mSurfaceWidth = surfaceWidth;
        mSurfaceHeight = surfaceHeight;
        mInstance = vulkanInstance;
        mSurface = surface;
        try {
			ResourceManager::initialize(200 * 1000 * 1000);
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

	void VulkanApp::initFramebuffers() {
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, nullptr);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, nullptr);
        mSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, mSwapchainImages.data());

        mImageViews.resize(mSwapchainImages.size());
        for (size_t i = 0; i < mSwapchainImages.size(); i++) {
            mImageViews[i] = createImageView(mDevice, mSwapchainImages[i], mSwapchain.swapchainImageFormat);
        }

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
	}

	void VulkanApp::cleanupSwapchain() {
		vkDestroyImageView(mDevice, mDepthView, nullptr);
		vmaDestroyImage(mAllocator, mDepthResource.memoryResource, mDepthResource.allocation);
		for (auto& imageView : mImageViews) {
			vkDestroyImageView(mDevice, imageView, nullptr);
		}
		for (auto& framebuffer : mFramebuffers) {
			vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
		}
		vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
		vkDestroySwapchainKHR(mDevice, mSwapchain.swapchain, nullptr);
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

		//mRenderFinished = mRenderer.drawFrame(mFramebuffers[imageIndex], &mImageAvailable, 1);
		//mRenderer.updateRenderPass(mRenderPass, mSwapchain.swapchainExtent);
		VkViewport viewport = {};
		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.width = (float)mSwapchain.swapchainExtent.width;
		viewport.height = (float)mSwapchain.swapchainExtent.height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = mSwapchain.swapchainExtent;
		AmbientLight ambient = {
			glm::vec3(1.f, 1.f, 1.f),
			0.05f
		};
		assert(vkWaitForFences(mDevice, 1, &mRenderFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);
		assert(vkResetFences(mDevice, 1, &mRenderFence) == VK_SUCCESS);
		std::array<VkCommandBuffer, 3> commandBuffers;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		commandBegin.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderBegin.renderPass = mRenderPass;
		renderBegin.framebuffer = mFramebuffers[imageIndex];
		renderBegin.renderArea = scissor;
		renderBegin.clearValueCount = static_cast<float>(clearValues.size());
		renderBegin.pClearValues = clearValues.data();

		vkBeginCommandBuffer(mPrimaryCommandBuffer, &commandBegin);
		vkCmdBeginRenderPass(mPrimaryCommandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        VkCommandBufferInheritanceInfo inheritanceInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
        inheritanceInfo.pNext = nullptr;
        inheritanceInfo.renderPass = mRenderPass;
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = mFramebuffers[imageIndex];
        inheritanceInfo.occlusionQueryEnable = VK_FALSE;
		
		commandBuffers[0] = mMeshRenderer->drawFrame(
            inheritanceInfo,
			mFramebuffers[imageIndex],
			viewport,
			scissor,
			*mActiveCamera.get(),
			ambient);

		commandBuffers[1] = mDebugRenderer->drawFrame(
			inheritanceInfo, 
			mFramebuffers[imageIndex], 
			viewport, 
			scissor,
			*mActiveCamera.get());

		commandBuffers[2] = mUiRenderer->drawFrame(
            inheritanceInfo,
			mFramebuffers[imageIndex],
			viewport,
			scissor);

		vkCmdExecuteCommands(mPrimaryCommandBuffer, static_cast<float>(commandBuffers.size()), commandBuffers.data());
		vkCmdEndRenderPass(mPrimaryCommandBuffer);
		vkEndCommandBuffer(mPrimaryCommandBuffer);

		/**************
		 submit to graphics queue
		 *************/
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &mImageAvailable;
		submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mPrimaryCommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &mRenderFinished;

		assert(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mRenderFence) == VK_SUCCESS);

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

    bool VulkanApp::update(double frameTime)
    {
        drawFrame();
        return false;
    }

    void VulkanApp::setActiveCamera(HVK_shared<Camera> camera)
    {
        mActiveCamera = camera;
    }

    void VulkanApp::setGammaCorrection(float gamma)
    {
        mMeshRenderer->setGammaCorrection(gamma);
    }

    void VulkanApp::setUseSRGBTex(bool useSRGBTex)
    {
        mMeshRenderer->setUseSRGBTex(useSRGBTex);
    }
}