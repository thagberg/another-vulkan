#include "UserApp.h"
#include "vulkanapp.h"
#include "InputManager.h"
#include "imgui\imgui.h"
#include "vulkan-util.h"
#include "GpuManager.h"
#include "types.h"
#include "HvkUtil.h"
#include "renderpass-util.h"
#include "image-util.h"
#include "command-util.h"
#include "framebuffer-util.h"
#include "Camera.h"
#include "LightTypes.h"

const uint32_t HEIGHT = 1024;
const uint32_t WIDTH = 1024;

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

namespace hvk
{

    UserApp::UserApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
        mWindowWidth(windowWidth),
        mWindowHeight(windowHeight),
        mWindowTitle(windowTitle),
        mVulkanInstance(VK_NULL_HANDLE),
        mWindowSurface(VK_NULL_HANDLE),
        mApp(std::make_unique<hvk::VulkanApp>()),
        mWindow(initializeWindow(mWindowWidth, mWindowHeight, mWindowTitle), glfwDestroyWindow),
        mClock(),
        mRegistry(),
        mPBRRenderPass(VK_NULL_HANDLE),
        mFinalRenderPass(VK_NULL_HANDLE),
        mSwapchain(),
        mSwapchainImages(),
        mSwapchainViews(),
        mSwapFramebuffers(),
        mPBRFramebuffer(VK_NULL_HANDLE),
        mPBRPassMap(nullptr),
        mPBRDepthImage(),
        mPBRDepthView(VK_NULL_HANDLE),
        mPBRMeshRenderer(nullptr),
        mUiRenderer(nullptr),
        mDebugRenderer(nullptr),
        mSkyboxRenderer(nullptr),
        mQuadRenderer(nullptr),
        mEnvironmentMap(nullptr),
        mIrradianceMap(nullptr),
        mPrefilteredMap(nullptr),
        mBrdfLutMap(nullptr),
        mGammaSettings(),
        mPBRWeight(),
        mExposureSettings(),
        mSkySettings(),
        mCamera(nullptr),
        mAmbientLight{glm::vec3(1.f), 0.3f},
        mSceneEntity(mRegistry.create()),
        mSkyEntity(mRegistry.create())
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

        mApp->init(mVulkanInstance, mWindowSurface);

        // Create swapchain
        assert(createSwapchain(
            GpuManager::getPhysicalDevice(),
            GpuManager::getDevice(),
            mWindowSurface,
            mWindowWidth,
            mWindowHeight,
            mSwapchain) == VK_SUCCESS);

        // initialize render passes
        createFinalRenderPass();
        createPBRRenderPass();

        // initialize framebuffers
        createPBRFramebuffers();
        createSwapFramebuffers();

        // Initialize various rendering settings
		// Initialize gamma settings
		mGammaSettings = { 2.2f };
		// Initialize PBR weight values
		mPBRWeight = { 1.f, 1.f };
		// Initialize Exposure settings
		mExposureSettings = { 1.0 };
		// Initialize sky settings
		mSkySettings = { 2.2f, 2.f };

        //// Create cubemap for skybox and environmental mapping
        //std::array<std::string, 6> skyboxFiles = {
        //    "resources/sky/desertsky_rt.png",
        //    "resources/sky/desertsky_lf.png",
        //    "resources/sky/desertsky_up_fixed.png",
        //    "resources/sky/desertsky_dn_fixed.png",
        //    "resources/sky/desertsky_bk.png",
        //    "resources/sky/desertsky_ft.png"
        //};
        //auto skyboxMap = HVK_make_shared<TextureMap>(util::image::createCubeMap(
        //    mDevice,
        //    mAllocator,
        //    mCommandPool,
        //    mGraphicsQueue,
        //    skyboxFiles));

		// Initialize lighting maps
		mEnvironmentMap = std::make_shared<TextureMap>();
		mIrradianceMap = std::make_shared<TextureMap>();
		mPrefilteredMap = std::make_shared<TextureMap>();
		mBrdfLutMap = std::make_shared<TextureMap>();
        mApp->generateEnvironmentMap(
            mEnvironmentMap,
            mIrradianceMap,
            mPrefilteredMap,
            mBrdfLutMap,
            mGammaSettings);

        // Initialize drawlist generators
		std::array<std::string, 2> quadShaderFiles = {
			"shaders/compiled/quad_vert.spv",
			"shaders/compiled/quad_frag.spv" };
        mQuadRenderer = std::make_shared<QuadGenerator>(
            mFinalRenderPass,
            GpuManager::getCommandPool(),
            mPBRPassMap,
			quadShaderFiles);

		mPBRMeshRenderer = std::make_shared<StaticMeshGenerator>(
            mPBRRenderPass, 
            GpuManager::getCommandPool(),
			mPrefilteredMap,
			mIrradianceMap,
			mBrdfLutMap);

		mUiRenderer = std::make_shared<UiDrawGenerator>(
            mFinalRenderPass, 
            GpuManager::getCommandPool(),
            mSwapchain.swapchainExtent);

		mDebugRenderer = std::make_shared<DebugDrawGenerator>(
            mPBRRenderPass, 
            GpuManager::getCommandPool());

		//std::array<std::string, 2> skyboxShaders = {
		//	"shaders/compiled/sky_vert.spv",
		//	"shaders/compiled/sky_frag.spv"};
		//mSkyboxRenderer = HVK_make_shared<CubemapGenerator<SkySettings>>(
		//	device,
		//	mAllocator,
		//	mGraphicsQueue,
		//	mColorRenderPass,
		//	mCommandPool,
  //          skyboxMap,
		//	skyboxShaders);

        mCamera = std::make_shared<Camera>(
            90.f,
            //WIDTH / static_cast<float>(HEIGHT),
            HEIGHT / static_cast<float>(WIDTH),
            0.001f,
            1000.f,
			"Main Camera",
			nullptr,
            glm::mat4(1.f));
    }

    UserApp::~UserApp()
    {
        glfwTerminate();
        mApp.reset();
        vkDestroySurfaceKHR(mVulkanInstance, mWindowSurface, nullptr);
        vkDestroyInstance(mVulkanInstance, nullptr);
    }

    void UserApp::createPBRFramebuffers()
    {
		mPBRPassMap = std::make_shared<TextureMap>(util::image::createImageMap(
            GpuManager::getDevice(),
            GpuManager::getAllocator(),
            GpuManager::getCommandPool(),
            GpuManager::getGraphicsQueue(),
			VK_FORMAT_R16G16B16A16_SFLOAT,
			mSwapchain.swapchainExtent.width,
			mSwapchain.swapchainExtent.height,
			0,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));

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
            GpuManager::getAllocator(),
            &depthImageCreate,
            &depthImageAllocationCreate,
            &mPBRDepthImage.memoryResource,
            &mPBRDepthImage.allocation,
            nullptr);

		mPBRDepthView = util::image::createImageView(
            GpuManager::getDevice(),
            mPBRDepthImage.memoryResource, 
            VK_FORMAT_D32_SFLOAT, 
            VK_IMAGE_ASPECT_DEPTH_BIT);
		auto commandBuffer = util::command::beginSingleTimeCommand(GpuManager::getDevice(), GpuManager::getCommandPool());
		util::image::transitionImageLayout(
			commandBuffer,
			mPBRDepthImage.memoryResource, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		util::command::endSingleTimeCommand(
            GpuManager::getDevice(), 
            GpuManager::getCommandPool(), 
            commandBuffer, 
            GpuManager::getGraphicsQueue());

        util::framebuffer::createFramebuffer(
            GpuManager::getDevice(),
            mPBRRenderPass,
			mSwapchain.swapchainExtent, 
			mPBRPassMap->view, 
			&mPBRDepthView, 
			&mPBRFramebuffer);
    }

    void UserApp::createPBRRenderPass()
    {
        // PBR pass dependencies
		std::vector<VkSubpassDependency> pbrPassDependencies = {
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

        // Color Attachment
		auto colorAttachment = util::renderpass::createColorAttachment(
			VK_FORMAT_R16G16B16A16_SFLOAT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Depth attachment
		auto depthAttachment = util::renderpass::createDepthAttachment();

        mPBRRenderPass = util::renderpass::createRenderPass(
            GpuManager::getDevice(),
            VK_FORMAT_R16G16B16A16_SFLOAT,
            pbrPassDependencies,
            &colorAttachment,
            &depthAttachment);
    }

    void UserApp::createFinalRenderPass()
    {
        auto finalColorAttachment = util::renderpass::createColorAttachment(mSwapchain.swapchainImageFormat);
		std::vector<VkSubpassDependency> finalPassDependencies = { util::renderpass::createSubpassDependency() };
        mFinalRenderPass = util::renderpass::createRenderPass(
            GpuManager::getDevice(),
            mSwapchain.swapchainImageFormat, 
            finalPassDependencies, 
            &finalColorAttachment);
    }

    void UserApp::createSwapFramebuffers()
    {
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(GpuManager::getDevice(), mSwapchain.swapchain, &imageCount, nullptr);
        vkGetSwapchainImagesKHR(GpuManager::getDevice(), mSwapchain.swapchain, &imageCount, nullptr);
        mSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(GpuManager::getDevice(), mSwapchain.swapchain, &imageCount, mSwapchainImages.data());

        mSwapchainViews.resize(mSwapchainImages.size());
        for (size_t i = 0; i < mSwapchainViews.size(); ++i)
        {
            mSwapchainViews[i] = util::image::createImageView(
                GpuManager::getDevice(),
                mSwapchainImages[i],
                mSwapchain.swapchainImageFormat);
        }

        mSwapFramebuffers.resize(mSwapchainViews.size());
        for (size_t i = 0; i < mSwapchainViews.size(); ++i)
        {
            util::framebuffer::createFramebuffer(
                GpuManager::getDevice(),
				mFinalRenderPass, 
				mSwapchain.swapchainExtent, 
				mSwapchainViews[i], 
				nullptr, 
				&mSwapFramebuffers[i]);
        }
    }

    void UserApp::handleWindowResize(GLFWwindow* window, int width, int height)
    {
        UserApp* thisApp = reinterpret_cast<UserApp*>(glfwGetWindowUserPointer(window));
        thisApp->mWindowWidth = width;
        thisApp->mWindowHeight = height;
        //thisApp->mApp->recreateSwapchain(thisApp->mWindowWidth, thisApp->mWindowHeight);

        vkDeviceWaitIdle(GpuManager::getDevice());
        thisApp->recreateSwapchain();
    }

    void UserApp::recreateSwapchain()
    {
        // invalidate renderers
        mPBRMeshRenderer->invalidate();
        mUiRenderer->invalidate();
        mDebugRenderer->invalidate();
        //mSkyboxRenderer->invalidate();
        mQuadRenderer->invalidate();

        cleanupSwapchain();
        createSwapchain(
            GpuManager::getPhysicalDevice(), 
            GpuManager::getDevice(), 
            mWindowSurface, 
            mWindowWidth, 
            mWindowHeight, 
            mSwapchain);

        // initialize render passes
        createFinalRenderPass();
        createPBRRenderPass();

        // initialize framebuffers
        createPBRFramebuffers();
        createSwapFramebuffers();

        // update camera FOV
        mCamera->updateProjection(
            90.0f,
            mSwapchain.swapchainExtent.width / static_cast<float>(mSwapchain.swapchainExtent.height),
            0.001f,
            1000.f);

        // revalidate renderers
        mPBRMeshRenderer->updateRenderPass(mPBRRenderPass);
        mUiRenderer->updateRenderPass(mFinalRenderPass, VkExtent2D{mWindowWidth, mWindowHeight});
        mDebugRenderer->updateRenderPass(mPBRRenderPass);
        //mSkyboxRenderer->updateRenderPass(mPBRRenderPass);
        mQuadRenderer->updateRenderPass(mFinalRenderPass, mPBRPassMap);
    }

    void UserApp::handleCharInput(GLFWwindow* window, uint32_t character)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharacter(character);
    }

	void UserApp::cleanupSwapchain()
	{
		const auto& device = GpuManager::getDevice();
		const auto& allocator = GpuManager::getAllocator();

		vkDestroyImageView(device, mPBRDepthView, nullptr);
		vmaDestroyImage(allocator, mPBRDepthImage.memoryResource, mPBRDepthImage.allocation);

        // destroy final framebuffers and renderpass
		for (auto& imageView : mSwapchainViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}
		for (auto& framebuffer : mSwapFramebuffers)
		{
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
		vkDestroyRenderPass(device, mFinalRenderPass, nullptr);

        // destroy PBR framebuffer and renderpass
		util::image::destroyMap(GpuManager::getDevice(), GpuManager::getAllocator(), *mPBRPassMap);
        vkDestroyFramebuffer(device, mPBRFramebuffer, nullptr);
        vkDestroyRenderPass(device, mPBRRenderPass, nullptr);

		vkDestroySwapchainKHR(device, mSwapchain.swapchain, nullptr);
	}

    void UserApp::drawFrame(double frametime)
    {
        uint32_t swapIndex = mApp->renderPrepare(mSwapchain.swapchain);

        // prepare PBR render pass
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.f };
        clearValues[1].depthStencil = { 1.f, 0 };

        VkRect2D scissor = {
            {0, 0},
            mSwapchain.swapchainExtent};

        // 3D scene is rendered using a "negative" viewport to compensate for Vulkan's coordinate system
        VkViewport viewport = {
            0.f, // x
            static_cast<float>(mSwapchain.swapchainExtent.height), // y
            static_cast<float>(mSwapchain.swapchainExtent.width), // width
            -static_cast<float>(mSwapchain.swapchainExtent.height), // height
            0.f, // minDepth
            1.f  // maxDepth
        };

        // Quad is rendered using a "normal" viewport
        VkViewport quadViewport = {
            0.f, // x
            0.f, // y
            static_cast<float>(mSwapchain.swapchainExtent.width), // width
            static_cast<float>(mSwapchain.swapchainExtent.height), // height
            0.f, // minDepth
            1.f  // maxDepth
        };

        VkRenderPassBeginInfo pbrRenderBegin = 
        { 
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            mPBRRenderPass,
            mPBRFramebuffer,
            scissor,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()
        };

        auto pbrInheritanceInfo = mApp->renderpassBegin(pbrRenderBegin);
        std::vector<VkCommandBuffer> pbrCommandBuffers;
        //pbrCommandBuffers.push_back(mPBRMeshRenderer->drawEl)
        auto pbrGroup = mRegistry.group<PBRMesh, PBRBinding>(entt::get<WorldTransform>);
        auto lightGroup = mRegistry.group<>(entt::get<LightColor, LightAttenuation, WorldTransform>, entt::exclude<SpotLight>);
        auto spotlightGroup = mRegistry.group<>(entt::get<LightColor, LightAttenuation, SpotLight, WorldTransform>);
        const auto& skyLightComponents = mRegistry.get<LightColor, Direction>(mSkyEntity);
        pbrCommandBuffers.push_back(mPBRMeshRenderer->drawElements(
            pbrInheritanceInfo,
            viewport,
            scissor,
            *mCamera,
            mAmbientLight,
            mGammaSettings,
            mPBRWeight,
            pbrGroup,
            lightGroup,
            spotlightGroup,
            skyLightComponents));

        auto debugGroup = mRegistry.group<DebugDrawMesh, DebugDrawBinding>(entt::get<WorldTransform>);
        pbrCommandBuffers.push_back(mDebugRenderer->drawElements(
            pbrInheritanceInfo,
            viewport,
            scissor,
            *mCamera,
            debugGroup));

        mApp->renderpassExecuteAndClose(pbrCommandBuffers);

        // prepare final render pass
        VkRenderPassBeginInfo finalRenderBegin = {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            mFinalRenderPass,
            mSwapFramebuffers[swapIndex],
            scissor,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()
        };
        auto finalInheritanceInfo = mApp->renderpassBegin(finalRenderBegin);
        std::vector<VkCommandBuffer> finalCommandBuffers;
        finalCommandBuffers.push_back(mQuadRenderer->drawFrame(
            finalInheritanceInfo,
            mSwapFramebuffers[swapIndex],
            quadViewport,
            scissor,
            mExposureSettings));

        finalCommandBuffers.push_back(mUiRenderer->drawFrame(
            finalInheritanceInfo,
            mSwapFramebuffers[swapIndex],
            viewport,
            scissor));

        mApp->renderpassExecuteAndClose(finalCommandBuffers);

        mApp->renderFinish();
        mApp->renderSubmit();
        mApp->renderPresent(swapIndex, mSwapchain.swapchain);
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
            drawFrame(frameTime);
            //shouldClose |= mApp->update(frameTime);
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

    hvk::ModelPipeline& UserApp::getModelPipeline()
    {
        return mApp->getModelPipeline();
    }
}
