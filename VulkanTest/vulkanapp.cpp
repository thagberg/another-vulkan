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
		mGraphicsQueue(VK_NULL_HANDLE),
        mColorRenderPass(VK_NULL_HANDLE),
        mFinalRenderPass(VK_NULL_HANDLE),
        mCommandPool(VK_NULL_HANDLE),
        mPrimaryCommandBuffer(VK_NULL_HANDLE),
        mFinalPassImageViews(),
        mFinalPassSwapchainImages(),
        mColorPassMap(nullptr),
        mDepthResource(),
		mDepthView(VK_NULL_HANDLE),
        mFinalPassFramebuffers(),
        mColorPassFramebuffer(VK_NULL_HANDLE),
        mSwapchain(),
        mImageAvailable(VK_NULL_HANDLE),
        mRenderFinished(VK_NULL_HANDLE),
		mFinalRenderFinished(VK_NULL_HANDLE),
        mAllocator(),
        mActiveCamera(nullptr),
        mMeshRenderer(nullptr),
        mUiRenderer(nullptr),
        mDebugRenderer(nullptr),
		mSkyboxRenderer(nullptr),
        mQuadRenderer(nullptr),
		mAmbientLight(nullptr),
        mRenderFence(VK_NULL_HANDLE),
		mSecondaryCommandBuffers(),
		mGammaSettings(nullptr)
    {

    }

    VulkanApp::~VulkanApp() {
        vkDeviceWaitIdle(mDevice);
        vkDestroySemaphore(mDevice, mImageAvailable, nullptr);
        vkDestroySemaphore(mDevice, mRenderFinished, nullptr);
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

		cleanupSwapchain();

        mQuadRenderer.reset();
        mMeshRenderer.reset();
        mUiRenderer.reset();
        mDebugRenderer.reset();
		mSkyboxRenderer.reset();

		destroyMap(mDevice, mAllocator, *mColorPassMap);
		vkDestroyFramebuffer(mDevice, mColorPassFramebuffer, nullptr);

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
		mFinalRenderFinished = createSemaphore(mDevice);
		assert(vkCreateFence(device.device, &fenceCreate, nullptr, &mRenderFence) == VK_SUCCESS);

		// Allocate command buffer
		VkCommandBufferAllocateInfo bufferAlloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		bufferAlloc.commandBufferCount = 1;
		bufferAlloc.commandPool = mCommandPool;
		bufferAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		assert(vkAllocateCommandBuffers(mDevice, &bufferAlloc, &mPrimaryCommandBuffer) == VK_SUCCESS);

		// Initialize gamma settings
		mGammaSettings = HVK_make_shared<GammaSettings>(GammaSettings{ 2.2f });

		// Initialize drawlist generators
        mQuadRenderer = HVK_make_shared<QuadGenerator>(
            device,
            mAllocator,
            mGraphicsQueue,
            mFinalRenderPass,
            mCommandPool,
            mColorPassMap);

		mMeshRenderer = std::make_shared<StaticMeshGenerator>(
            device, 
            mAllocator, 
            mGraphicsQueue, 
            mColorRenderPass, 
            mCommandPool);

		mUiRenderer = std::make_shared<UiDrawGenerator>(
            device, 
            mAllocator, 
            mGraphicsQueue, 
            mColorRenderPass, 
            mCommandPool, 
            mSwapchain.swapchainExtent);

		mDebugRenderer = std::make_shared<DebugDrawGenerator>(
            device, 
            mAllocator, 
            mGraphicsQueue, 
            mColorRenderPass, 
            mCommandPool);

		mSkyboxRenderer = HVK_make_shared<SkyboxGenerator>(
			device,
			mAllocator,
			mGraphicsQueue,
			mColorRenderPass,
			mCommandPool);

		mSecondaryCommandBuffers.resize(4);

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

		mSkyboxRenderer->invalidate();
		mMeshRenderer->invalidate();
		mUiRenderer->invalidate();
		mDebugRenderer->invalidate();
        mQuadRenderer->invalidate();
		cleanupSwapchain();

		// the color pass map which we render to in the color pass
		// needs to be recreated at the new size
		destroyMap(mDevice, mAllocator, *mColorPassMap);

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
		mSkyboxRenderer->updateRenderPass(mColorRenderPass);
		mMeshRenderer->updateRenderPass(mColorRenderPass);
		mUiRenderer->updateRenderPass(mColorRenderPass, mSwapchain.swapchainExtent);
		mDebugRenderer->updateRenderPass(mColorRenderPass);
        mQuadRenderer->updateRenderPass(mFinalRenderPass);
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
			mAmbientLight = HVK_make_shared<AmbientLight>(AmbientLight{
				glm::vec3(1.f, 1.f, 1.f),
				0.1f });
    	
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
        mFinalPassSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, mFinalPassSwapchainImages.data());

        mFinalPassImageViews.resize(mFinalPassSwapchainImages.size());
        for (size_t i = 0; i < mFinalPassSwapchainImages.size(); i++) {
            mFinalPassImageViews[i] = createImageView(mDevice, mFinalPassSwapchainImages[i], mSwapchain.swapchainImageFormat);
        }

		// if mColorPassMap already exists, then just overwrite the value there instead
		if (mColorPassMap == nullptr) {
			mColorPassMap = HVK_make_shared<TextureMap>(createImageMap(
				mDevice,
				mAllocator,
				mCommandPool,
				mGraphicsQueue,
				mSwapchain.swapchainImageFormat,
				mSwapchain.swapchainExtent.width,
				mSwapchain.swapchainExtent.height,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
		}
		else {
			*mColorPassMap =createImageMap(
				mDevice,
				mAllocator,
				mCommandPool,
				mGraphicsQueue,
				mSwapchain.swapchainImageFormat,
				mSwapchain.swapchainExtent.width,
				mSwapchain.swapchainExtent.height,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
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


		//auto colorAttachment = createColorAttachment(mSwapchain.swapchainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		auto colorAttachment = createColorAttachment(mSwapchain.swapchainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		auto depthAttachment = createDepthAttachment();
		std::vector<VkSubpassDependency> colorPassDependencies = {
			createSubpassDependency(
				VK_SUBPASS_EXTERNAL,
				0,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_DEPENDENCY_BY_REGION_BIT),
			createSubpassDependency(
				0,
				VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_DEPENDENCY_BY_REGION_BIT)
		};
        mColorRenderPass = createRenderPass(mDevice, mSwapchain.swapchainImageFormat, colorPassDependencies,&colorAttachment, &depthAttachment);

        auto finalColorAttachment = createColorAttachment(mSwapchain.swapchainImageFormat);
		std::vector<VkSubpassDependency> finalPassDependencies = { createSubpassDependency() };
        mFinalRenderPass = createRenderPass(mDevice, mSwapchain.swapchainImageFormat, finalPassDependencies, &finalColorAttachment);

        //createFramebuffers(mDevice, mFinalPassImageViews, mDepthView, mRenderPass, mSwapchain.swapchainExtent, mFinalPassFramebuffers);
        //createFramebuffers(mDevice, mFinalPassImageViews, mDepthView, mColorRenderPass, mSwapchain.swapchainExtent, mColorPassFramebuffers);

        mFinalPassFramebuffers.resize(mFinalPassImageViews.size());
		//mColorPassFramebuffers.resize(mFinalPassImageViews.size());
        createFramebuffer(mDevice, mColorRenderPass, mSwapchain.swapchainExtent, mColorPassMap->view, &mDepthView, &mColorPassFramebuffer);
        for (size_t i = 0; i < mFinalPassImageViews.size(); ++i)
        {
            //createFramebuffers(mDevice, mFinalPassImageViews, mDepthView, mColorRenderPass, mSwapchain.swapchainExtent, mColorPassFramebuffers);
            createFramebuffer(mDevice, mFinalRenderPass, mSwapchain.swapchainExtent, mFinalPassImageViews[i], nullptr, &mFinalPassFramebuffers[i]);
        }
	}

	void VulkanApp::cleanupSwapchain() {
		vkDestroyImageView(mDevice, mDepthView, nullptr);
		vmaDestroyImage(mAllocator, mDepthResource.memoryResource, mDepthResource.allocation);
		for (auto& imageView : mFinalPassImageViews) {
			vkDestroyImageView(mDevice, imageView, nullptr);
		}
		for (auto& framebuffer : mFinalPassFramebuffers) {
			vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
		}
		vkDestroyRenderPass(mDevice, mColorRenderPass, nullptr);
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
		assert(vkWaitForFences(mDevice, 1, &mRenderFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);
		assert(vkResetFences(mDevice, 1, &mRenderFence) == VK_SUCCESS);

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		commandBegin.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderBegin.renderPass = mColorRenderPass;
		renderBegin.framebuffer = mColorPassFramebuffer;
		renderBegin.renderArea = scissor;
		renderBegin.clearValueCount = static_cast<float>(clearValues.size());
		renderBegin.pClearValues = clearValues.data();

		VkRenderPassBeginInfo finalRenderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		finalRenderBegin.renderPass = mFinalRenderPass;
		finalRenderBegin.framebuffer = mFinalPassFramebuffers[imageIndex];
		finalRenderBegin.renderArea = scissor;
		finalRenderBegin.clearValueCount = static_cast<float>(clearValues.size());
		finalRenderBegin.pClearValues = clearValues.data();

        VkCommandBufferInheritanceInfo inheritanceInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
        inheritanceInfo.pNext = nullptr;
        inheritanceInfo.renderPass = mColorRenderPass;
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = mColorPassFramebuffer;
        inheritanceInfo.occlusionQueryEnable = VK_FALSE;

		vkBeginCommandBuffer(mPrimaryCommandBuffer, &commandBegin);

		// Color pass
		vkCmdBeginRenderPass(mPrimaryCommandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		mSecondaryCommandBuffers[0] = mMeshRenderer->drawFrame(
            inheritanceInfo,
			mColorPassFramebuffer,
			viewport,
			scissor,
			*mActiveCamera.get(),
			*mAmbientLight,
			*mGammaSettings);

		mSecondaryCommandBuffers[1] = mDebugRenderer->drawFrame(
			inheritanceInfo, 
			mColorPassFramebuffer, 
			viewport, 
			scissor,
			*mActiveCamera.get());

		mSecondaryCommandBuffers[2] = mSkyboxRenderer->drawFrame(
			inheritanceInfo,
			mColorPassFramebuffer,
			viewport,
			scissor,
			*mActiveCamera,
			*mGammaSettings);
		

		mSecondaryCommandBuffers[3] = mUiRenderer->drawFrame(
            inheritanceInfo,
			mColorPassFramebuffer,
			viewport,
			scissor);

		vkCmdExecuteCommands(
			mPrimaryCommandBuffer, 
			static_cast<float>(mSecondaryCommandBuffers.size()), 
			mSecondaryCommandBuffers.data());
		vkCmdEndRenderPass(mPrimaryCommandBuffer);

		inheritanceInfo.renderPass = mFinalRenderPass;
		inheritanceInfo.framebuffer = mFinalPassFramebuffers[imageIndex];

		// Final pass -- renders previous color attachment to quad
		vkCmdBeginRenderPass(mPrimaryCommandBuffer, &finalRenderBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        auto finalCommandBuffer = mQuadRenderer->drawFrame(
            inheritanceInfo,
            mFinalPassFramebuffers[imageIndex],
            viewport,
            scissor);

        vkCmdExecuteCommands(mPrimaryCommandBuffer, 1, &finalCommandBuffer);
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

	void VulkanApp::setAmbientLight(HVK_shared<AmbientLight> ambientLight)
    {
		mAmbientLight = ambientLight;
    }
}