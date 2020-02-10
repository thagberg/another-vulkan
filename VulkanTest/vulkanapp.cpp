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
        mInstance(VK_NULL_HANDLE),
        mDevice(VK_NULL_HANDLE),
        mPhysicalDevice(VK_NULL_HANDLE),
        mGraphicsIndex(),
		mGraphicsQueue(VK_NULL_HANDLE),
        mCommandPool(VK_NULL_HANDLE),
        mPrimaryCommandBuffer(VK_NULL_HANDLE),
        mModelPipeline(),
        mImageAvailable(VK_NULL_HANDLE),
        mRenderFinished(VK_NULL_HANDLE),
		mFinalRenderFinished(VK_NULL_HANDLE),
        mAllocator(),
        mRenderFence(VK_NULL_HANDLE)
    {

    }

    VulkanApp::~VulkanApp() {
        vkDeviceWaitIdle(mDevice);
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

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

    void VulkanApp::init(
            VkInstance vulkanInstance,
            VkSurfaceKHR surface)
    {
        mInstance = vulkanInstance;

		// load Renderdoc API
		//HMODULE renderMod = LoadLibraryA("C:\\\\Program Files\\RenderDoc\\renderdoc.dll");
		//if (HMODULE mod = GetModuleHandleA("C:\\\\Program Files\\RenderDoc\\renderdoc.dll"))
		//{
		//	pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
		//	int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_0, (void**)&rdoc_api);
		//	assert(ret == 1);
		//}

        try {
            enableVulkanValidationLayers();
            std::cout << "init device" << std::endl;
            initializeDevice();
			std::cout << "Verify surface support and compatibility" << std::endl;
			VkBool32 surfaceSupported;
			vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, mGraphicsIndex, surface, &surfaceSupported);
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


	uint32_t VulkanApp::renderPrepare(VkSwapchainKHR& swapchain)
	{
		// Wait for an image on the swapchain to become available
        uint32_t imageIndex;
        vkAcquireNextImageKHR(
            mDevice,
            swapchain,
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

	void VulkanApp::renderpassExecuteAndClose(const std::vector<VkCommandBuffer>& secondaryBuffers)
	{
		vkCmdExecuteCommands(mPrimaryCommandBuffer, static_cast<uint32_t>(secondaryBuffers.size()), secondaryBuffers.data());
		vkCmdEndRenderPass(mPrimaryCommandBuffer);
	}

	void VulkanApp::renderpassExecute(const std::vector<VkCommandBuffer>& secondaryBuffers)
	{
		vkCmdExecuteCommands(mPrimaryCommandBuffer, static_cast<uint32_t>(secondaryBuffers.size()), secondaryBuffers.data());
	}

	void VulkanApp::renderpassClose()
	{
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

	void VulkanApp::renderPresent(uint32_t swapIndex, VkSwapchainKHR swapchain)
	{
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mRenderFinished;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &swapIndex;
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(mGraphicsQueue, &presentInfo);
	}

    bool VulkanApp::update(double frameTime)
    {
        return false;
    }

	void VulkanApp::generateEnvironmentMap(
		std::shared_ptr<TextureMap> environmentMap,
		std::shared_ptr<TextureMap> irradianceMap,
		std::shared_ptr<TextureMap> prefilteredMap,
		std::shared_ptr<TextureMap> brdfLutMap,
        const GammaSettings& gammaSettings)
	{
        const auto& device = GpuManager::getDevice();
        const auto& allocator = GpuManager::getAllocator();
        const auto& commandPool = GpuManager::getCommandPool();
        const auto& graphicsQueue = GpuManager::getGraphicsQueue();

		// Convert HDR equirectangular map to cubemap
		int hdrWidth, hdrHeight, hdrBitDepth;
		float* hdrData = stbi_loadf("resources/Alexs_Apartment/Alexs_Apt_2k.hdr", &hdrWidth, &hdrHeight, &hdrBitDepth, 4);
		//float* hdrData = stbi_loadf("resources/MonValley_Lookout/MonValley_A_LookoutPoint_2k.hdr", &hdrWidth, &hdrHeight, &hdrBitDepth, 4);
		auto hdrImage = util::image::createTextureImage(
            device,
            allocator,
            commandPool,
            graphicsQueue,
			hdrData, 
			1, 
			hdrWidth, 
			hdrHeight, 
			4 * 4,  // hdrBitDepth might be 3, but we are telling stb_image to fake the Alpha channel and floats are 2 bytes
			VK_IMAGE_TYPE_2D, 
			0, 
			VK_FORMAT_R32G32B32A32_SFLOAT);
		auto hdrMap = std::make_shared<TextureMap>(TextureMap{
			hdrImage,
			util::image::createImageView(mDevice, hdrImage.memoryResource, VK_FORMAT_R32G32B32A32_SFLOAT),
			util::image::createImageSampler(mDevice)});

		std::vector<GammaSettings> vgammaSettings = { gammaSettings };

		std::array<std::string, 2> hdrMapShaders = {
			"shaders/compiled/hdr_to_cubemap_vert.spv",
			"shaders/compiled/hdr_to_cubemap_frag.spv"};
		util::render::renderCubeMap<GammaSettings>(
			device,
            allocator,
            commandPool,
            graphicsQueue,
			mPrimaryCommandBuffer,
			hdrMap,
			1024,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			environmentMap,
			hdrMapShaders,
			vgammaSettings);

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
            allocator,
            commandPool,
            graphicsQueue,
			mPrimaryCommandBuffer,
			environmentMap,
			32,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			irradianceMap,
			irradianceMapShaders,
			vgammaSettings);

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
            allocator,
            commandPool,
            graphicsQueue,
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
            allocator,
            commandPool,
            graphicsQueue,
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