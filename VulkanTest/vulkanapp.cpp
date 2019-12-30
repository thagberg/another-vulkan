#include "pch.h"
#include <vector>
#include <iostream>
#include <limits>

#include "stb_image.h"
#include "stb_image_write.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "vulkanapp.h"
#include "vulkan-util.h"
#include "RenderObject.h"
#include "InputManager.h"
#include "image-util.h"
#include "renderpass-util.h"
#include "framebuffer-util.h"
#include "signal-util.h"
#include "command-util.h"
#include "render-util.h"
#include "GpuManager.h"

#include "HvkUtil.h"

#include <renderdoc_app.h>

RENDERDOC_API_1_4_0* rdoc_api = nullptr;

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
        mModelPipeline(),
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
		mFirstPassCommandBuffers(),
		mSecondPassCommandBuffers(),
		mGammaSettings(nullptr),
		mPBRWeight(nullptr),
		mExposureSettings(nullptr),
		mSkySettings(nullptr),
		mEnvironmentMap(nullptr),
		mIrradianceMap(nullptr),
		mPrefilteredMap(nullptr),
		mBdrfLutMap(nullptr)
    {

    }

    VulkanApp::~VulkanApp() {
        vkDeviceWaitIdle(mDevice);
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

		cleanupSwapchain();

        mQuadRenderer.reset();
        mMeshRenderer.reset();
        mUiRenderer.reset();
        mDebugRenderer.reset();
		mSkyboxRenderer.reset();

		util::image::destroyMap(mDevice, mAllocator, *mColorPassMap);
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
        mCommandPool = util::command::createCommandPool(
			mDevice, 
			mGraphicsIndex, 
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		// Create fence for rendering complete
		VkFenceCreateInfo fenceCreate = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreate.pNext = nullptr;
		fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		mRenderFinished = util::signal::createSemaphore(mDevice);
		mFinalRenderFinished = util::signal::createSemaphore(mDevice);
		assert(vkCreateFence(mDevice, &fenceCreate, nullptr, &mRenderFence) == VK_SUCCESS);

		// Allocate command buffer
		VkCommandBufferAllocateInfo bufferAlloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		bufferAlloc.commandBufferCount = 1;
		bufferAlloc.commandPool = mCommandPool;
		bufferAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		assert(vkAllocateCommandBuffers(mDevice, &bufferAlloc, &mPrimaryCommandBuffer) == VK_SUCCESS);

        mImageAvailable = util::signal::createSemaphore(mDevice);
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
		util::image::destroyMap(mDevice, mAllocator, *mColorPassMap);

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
		mDebugRenderer->updateRenderPass(mColorRenderPass);
        mQuadRenderer->updateRenderPass(mFinalRenderPass);
		mUiRenderer->updateRenderPass(mFinalRenderPass, mSwapchain.swapchainExtent);
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


		// load Renderdoc API
		HMODULE renderMod = LoadLibraryA("C:\\\\Program Files\\RenderDoc\\renderdoc.dll");
		if (HMODULE mod = GetModuleHandleA("C:\\\\Program Files\\RenderDoc\\renderdoc.dll"))
		{
			pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
			int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_0, (void**)&rdoc_api);
			assert(ret == 1);
		}

        try {
			ResourceManager::initialize(200 * 1000 * 1000);
			mAmbientLight = HVK_make_shared<AmbientLight>(AmbientLight{
				glm::vec3(1.f, 1.f, 1.f),
				0.1f });
    	
            enableVulkanValidationLayers();
            std::cout << "init device" << std::endl;
            initializeDevice();
			std::cout << "Verify surface support and compatibility" << std::endl;
			VkBool32 surfaceSupported;
			vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, mGraphicsIndex, mSurface, &surfaceSupported);
			if (!surfaceSupported) {
				throw std::runtime_error("Surface not supported by Physical Device");
			}
            std::cout << "init renderer" << std::endl;
            initializeRenderer();
            std::cout << "done initializing" << std::endl;
        }
        catch (const std::runtime_error& error) {
            std::cout << "Error during initialization: " << error.what() << std::endl;
        }

		GpuManager::init(mPhysicalDevice, mDevice, mCommandPool, mGraphicsQueue, mAllocator);
        mModelPipeline.init();
    }

	void VulkanApp::initFramebuffers() {
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, nullptr);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, nullptr);
        mFinalPassSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, mFinalPassSwapchainImages.data());

        mFinalPassImageViews.resize(mFinalPassSwapchainImages.size());
        for (size_t i = 0; i < mFinalPassSwapchainImages.size(); i++) {
            mFinalPassImageViews[i] = util::image::createImageView(mDevice, mFinalPassSwapchainImages[i], mSwapchain.swapchainImageFormat);
        }

		// if mColorPassMap already exists, then just overwrite the value there instead
		if (mColorPassMap == nullptr) {
			mColorPassMap = HVK_make_shared<TextureMap>(util::image::createImageMap(
				mDevice,
				mAllocator,
				mCommandPool,
				mGraphicsQueue,
				VK_FORMAT_R16G16B16A16_SFLOAT,
				mSwapchain.swapchainExtent.width,
				mSwapchain.swapchainExtent.height,
				0,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
		}
		else {
			*mColorPassMap = util::image::createImageMap(
				mDevice,
				mAllocator,
				mCommandPool,
				mGraphicsQueue,
				VK_FORMAT_R16G16B16A16_SFLOAT,
				mSwapchain.swapchainExtent.width,
				mSwapchain.swapchainExtent.height,
				0,
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

		mDepthView = util::image::createImageView(mDevice, mDepthResource.memoryResource, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
		auto commandBuffer = util::command::beginSingleTimeCommand(mDevice, mCommandPool);
		util::image::transitionImageLayout(
			commandBuffer,
			mDepthResource.memoryResource, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		util::command::endSingleTimeCommand(mDevice, mCommandPool, commandBuffer, mGraphicsQueue);


		auto colorAttachment = util::renderpass::createColorAttachment(
			VK_FORMAT_R16G16B16A16_SFLOAT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		auto depthAttachment = util::renderpass::createDepthAttachment();
		std::vector<VkSubpassDependency> colorPassDependencies = {
			util::renderpass::createSubpassDependency(
				VK_SUBPASS_EXTERNAL,
				0,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_DEPENDENCY_BY_REGION_BIT),
			util::renderpass::createSubpassDependency(
				0,
				VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_DEPENDENCY_BY_REGION_BIT)
		};
        mColorRenderPass = util::renderpass::createRenderPass(
			mDevice, 
			VK_FORMAT_R16G16B16A16_SFLOAT, 
			colorPassDependencies, 
			&colorAttachment, 
			&depthAttachment);

        auto finalColorAttachment = util::renderpass::createColorAttachment(mSwapchain.swapchainImageFormat);
		std::vector<VkSubpassDependency> finalPassDependencies = { util::renderpass::createSubpassDependency() };
        mFinalRenderPass = util::renderpass::createRenderPass(mDevice, mSwapchain.swapchainImageFormat, finalPassDependencies, &finalColorAttachment);

        mFinalPassFramebuffers.resize(mFinalPassImageViews.size());
        util::framebuffer::createFramebuffer(
			mDevice, 
			mColorRenderPass, 
			mSwapchain.swapchainExtent, 
			mColorPassMap->view, 
			&mDepthView, 
			&mColorPassFramebuffer);
        for (size_t i = 0; i < mFinalPassImageViews.size(); ++i)
        {
            util::framebuffer::createFramebuffer(
				mDevice, 
				mFinalRenderPass, 
				mSwapchain.swapchainExtent, 
				mFinalPassImageViews[i], 
				nullptr, 
				&mFinalPassFramebuffers[i]);
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

	uint32_t VulkanApp::renderPrepare()
	{
		// Wait for an image on the swapchain to become available
        uint32_t imageIndex;
        vkAcquireNextImageKHR(
            mDevice,
            mSwapchain.swapchain,
            std::numeric_limits<uint64_t>::max(),
            mImageAvailable,
            VK_NULL_HANDLE,
            &imageIndex);

		// Wait for render fence to free
		assert(vkWaitForFences(mDevice, 1, &mRenderFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);
		assert(vkResetFences(mDevice, 1, &mRenderFence) == VK_SUCCESS);

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		commandBegin.pInheritanceInfo = nullptr;
		assert(vkBeginCommandBuffer(mPrimaryCommandBuffer, &commandBegin) == VK_SUCCESS);

		return imageIndex;
	}

	VkCommandBufferInheritanceInfo VulkanApp::renderpassBegin(const VkRenderPassBeginInfo& renderBegin)
	{
		VkCommandBufferInheritanceInfo inheritanceInfo = 
		{ 
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,					// pNext
			renderBegin.renderPass,		// renderpass
			0,							// subpass
			renderBegin.framebuffer,	// framebuffer
			VK_FALSE,					// occlusionQueryEnable
			0,							// queryFlags
			0,							// pipelineStatistics
		};
		vkCmdBeginRenderPass(mPrimaryCommandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		return inheritanceInfo;
	}

	void VulkanApp::renderpassExecuteAndClose(std::vector<VkCommandBuffer>& secondaryBuffers)
	{
		vkCmdExecuteCommands(mPrimaryCommandBuffer, static_cast<uint32_t>(secondaryBuffers.size()), secondaryBuffers.data());
		vkCmdEndRenderPass(mPrimaryCommandBuffer);
	}

	void VulkanApp::renderFinish()
	{
		assert(vkEndCommandBuffer(mPrimaryCommandBuffer) == VK_SUCCESS);
	}

	void VulkanApp::renderSubmit()
	{
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
	}

	void VulkanApp::renderPresent(uint32_t swapIndex)
	{
        VkSwapchainKHR swapchains[] = { mSwapchain.swapchain };
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mRenderFinished;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &swapIndex;
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(mGraphicsQueue, &presentInfo);
	}

    void VulkanApp::drawFrame() {

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

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		uint32_t swapIndex = renderPrepare();

		VkRenderPassBeginInfo renderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderBegin.renderPass = mColorRenderPass;
		renderBegin.framebuffer = mColorPassFramebuffer;
		renderBegin.renderArea = scissor;
		renderBegin.clearValueCount = static_cast<float>(clearValues.size());
		renderBegin.pClearValues = clearValues.data();

		auto inheritanceInfo = renderpassBegin(renderBegin);

		std::vector<VkCommandBuffer> colorpassCommandBuffers;
		colorpassCommandBuffers.push_back(mMeshRenderer->drawFrame(
            inheritanceInfo,
			mColorPassFramebuffer,
			viewport,
			scissor,
			*mActiveCamera.get(),
			*mAmbientLight,
			*mGammaSettings,
			*mPBRWeight));

		colorpassCommandBuffers.push_back(mDebugRenderer->drawFrame(
			inheritanceInfo, 
			mColorPassFramebuffer, 
			viewport, 
			scissor,
			*mActiveCamera.get()));

		SkySettings tempSkySettings{
			mGammaSettings->gamma,
			mSkySettings->lod
		};
		colorpassCommandBuffers.push_back(mSkyboxRenderer->drawFrame(
			inheritanceInfo,
			mColorPassFramebuffer,
			viewport,
			scissor,
			*mActiveCamera,
			tempSkySettings));

		renderpassExecuteAndClose(colorpassCommandBuffers);

		renderBegin.renderPass = mFinalRenderPass;
		renderBegin.framebuffer = mFinalPassFramebuffers[swapIndex];
		inheritanceInfo = renderpassBegin(renderBegin);

		std::vector<VkCommandBuffer> finalpassCommandBuffers;
        finalpassCommandBuffers.push_back(mQuadRenderer->drawFrame(
            inheritanceInfo,
            mFinalPassFramebuffers[swapIndex],
            viewport,
            scissor,
			*mExposureSettings));

		finalpassCommandBuffers.push_back(mUiRenderer->drawFrame(
            inheritanceInfo,
			//mColorPassFramebuffer,
			mFinalPassFramebuffers[swapIndex],
			viewport,
			scissor));

		renderpassExecuteAndClose(finalpassCommandBuffers);

		// end primary command buffer, execute commands, and then present to screen
		renderFinish();
		renderSubmit();
		renderPresent(swapIndex);
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

	void VulkanApp::generateEnvironmentMap(
		std::shared_ptr<TextureMap> environmentMap,
		std::shared_ptr<TextureMap> irradianceMap,
		std::shared_ptr<TextureMap> prefilteredMap,
		std::shared_ptr<TextureMap> brdfLutMap)
	{
		QueueFamilies families = {
			mGraphicsIndex,
			mGraphicsIndex
		};
		VulkanDevice device = {
			mPhysicalDevice,
			mDevice,
			families
		};
		// Convert HDR equirectangular map to cubemap
		int hdrWidth, hdrHeight, hdrBitDepth;
		float* hdrData = stbi_loadf("resources/Alexs_Apartment/Alexs_Apt_2k.hdr", &hdrWidth, &hdrHeight, &hdrBitDepth, 4);
		//float* hdrData = stbi_loadf("resources/MonValley_Lookout/MonValley_A_LookoutPoint_2k.hdr", &hdrWidth, &hdrHeight, &hdrBitDepth, 4);
		auto hdrImage = util::image::createTextureImage(
			mDevice, 
			mAllocator, 
			mCommandPool, 
			mGraphicsQueue, 
			hdrData, 
			1, 
			hdrWidth, 
			hdrHeight, 
			4 * 4,  // hdrBitDepth might be 3, but we are telling stb_image to fake the Alpha channel and floats are 2 bytes
			VK_IMAGE_TYPE_2D, 
			0, 
			VK_FORMAT_R32G32B32A32_SFLOAT);
		auto hdrMap = HVK_make_shared<TextureMap>(TextureMap{
			hdrImage,
			util::image::createImageView(mDevice, hdrImage.memoryResource, VK_FORMAT_R32G32B32A32_SFLOAT),
			util::image::createImageSampler(mDevice)});

		std::vector<GammaSettings> gammaSettings = { *mGammaSettings };

		std::array<std::string, 2> hdrMapShaders = {
			"shaders/compiled/hdr_to_cubemap_vert.spv",
			"shaders/compiled/hdr_to_cubemap_frag.spv"};
		util::render::renderCubeMap<GammaSettings>(
			device,
			mAllocator,
			mCommandPool,
			mGraphicsQueue,
			mPrimaryCommandBuffer,
			hdrMap,
			1024,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			environmentMap,
			hdrMapShaders,
			gammaSettings);

        // clean up
        vmaDestroyImage(mAllocator, hdrImage.memoryResource, hdrImage.allocation);

		// Finally, update the skybox
		//mSkyboxRenderer->setCubemap(cubemap);

		std::array<std::string, 2> irradianceMapShaders = {
			"shaders/compiled/convolution_vert.spv",
			"shaders/compiled/convolution_frag.spv"
		};
		util::render::renderCubeMap<GammaSettings>(
			device,
			mAllocator,
			mCommandPool,
			mGraphicsQueue,
			mPrimaryCommandBuffer,
			environmentMap,
			32,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			irradianceMap,
			irradianceMapShaders,
			gammaSettings);

		//mSkyboxRenderer->setCubemap(irradianceMap);
		
		uint32_t numMips = static_cast<uint32_t>(std::floor(std::log2(256.f))) + 1;
		std::vector<RoughnessSettings> roughnessSettings;
		roughnessSettings.reserve(numMips);
		for (uint32_t i = 0; i < numMips; ++i)
		{
			roughnessSettings.push_back(RoughnessSettings{ static_cast<float>(i) / numMips });
		}
		std::array<std::string, 2> prefilterMapShaders = {
			"shaders/compiled/prefiltered-environment_vert.spv",
			"shaders/compiled/prefiltered-environment_frag.spv"
		};
		util::render::renderCubeMap<RoughnessSettings>(
			device,
			mAllocator,
			mCommandPool,
			mGraphicsQueue,
			mPrimaryCommandBuffer,
			environmentMap,
			256,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			prefilteredMap,
			prefilterMapShaders,
			roughnessSettings,
			numMips);

		if (rdoc_api)
		{
			rdoc_api->StartFrameCapture(nullptr, nullptr);
		}
		std::array<std::string, 2> brdfLutShaders = {
			"shaders/compiled/quad_vert.spv",
			"shaders/compiled/brdfLUT_frag.spv" };
		auto brdfFence = util::render::renderImageMap(
			device,
			mAllocator,
			mCommandPool,
			mGraphicsQueue,
			mPrimaryCommandBuffer,
			512,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			brdfLutMap,
			brdfLutShaders);
		assert(vkWaitForFences(mDevice, 1, &brdfFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);

		if (rdoc_api)
		{
			rdoc_api->EndFrameCapture(nullptr, nullptr);
		}
	}
}