#include <math.h>
#include <algorithm>

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
#include "ShadowGenerator.h"
#include "math-util.h"

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
        mShadowRenderPass(VK_NULL_HANDLE),
        mSwapchain(),
        mSwapchainImages(),
        mSwapchainViews(),
        mSwapFramebuffers(),
        mPBRFramebuffer(VK_NULL_HANDLE),
        mPBRPassMap(nullptr),
        mPBRDepthImage(),
        mPBRDepthView(VK_NULL_HANDLE),
        mShadowFramebuffer(VK_NULL_HANDLE),
        mShadowDepthMap(nullptr),
        mShadowSampleMap(nullptr),
        mPBRMeshRenderer(nullptr),
        mUiRenderer(nullptr),
        mDebugRenderer(nullptr),
        mSkyboxRenderer(nullptr),
        mQuadRenderer(nullptr),
        mShadowRenderer(nullptr),
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
        mSkyEntity(mRegistry.create()),
        mLightClusters()
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
        createShadowRenderPass();

        // initialize framebuffers
        createPBRFramebuffers();
        createSwapFramebuffers();
        createShadowFramebuffer();

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

        mShadowRenderer = std::make_shared<ShadowGenerator>(
            mShadowRenderPass,
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
            60.f,
            //WIDTH / static_cast<float>(HEIGHT),
            HEIGHT / static_cast<float>(WIDTH),
            0.1f,
            1000.f,
			"Main Camera",
			nullptr,
            glm::mat4(1.f));

        // calculate AABBs for clusters
        const size_t X_TILES = 16;
        const size_t Y_TILES = 16;
        const size_t Z_SLICES = 24;
        mLightClusters.reserve(X_TILES* Y_TILES* Z_SLICES);

        // iterate from near slice to far slice
        float cameraNear = mCamera->getNear();
        float cameraFar = mCamera->getFar();
		const glm::mat4 inverseProjection = glm::inverse(mCamera->getProjection());
        const glm::vec2 screenDimensions = glm::vec2(WIDTH, HEIGHT);
        const float step = (cameraFar - cameraNear) / Z_SLICES;
        for (size_t slice = 0; slice < Z_SLICES; ++slice)
        {
            // For each slice, create tiles across and down the screen.
            // Cast a ray from the camera and through the camera near plane at
            // the corners of each tile, onto the near and far planes of the
            // slice volume.  Construct an AABB which encompasses the entirety
            // of each intersected cluster

            const float nearSliceDepth = -cameraNear * std::pow((cameraFar / cameraNear), (slice / static_cast<float>(Z_SLICES)));
            const float farSliceDepth = -cameraNear * std::pow((cameraFar / cameraNear), ((slice + 1) / static_cast<float>(Z_SLICES)));
            //const float nearSliceDepth = -(cameraNear + (slice*step));
            //const float farSliceDepth = -(cameraNear + ((slice+1)*step));
            util::math::Plane clusterNearPlane{ glm::vec3(0.f, 0.f, nearSliceDepth), glm::vec3(0.f, 0.f, 1.f) };
            util::math::Plane clusterFarPlane{ glm::vec3(0.f, 0.f, farSliceDepth), glm::vec3(0.f, 0.f, 1.f) };
            for (size_t x = 0; x < X_TILES; ++x)
            {
                const float xLeftScreen = WIDTH * (x / static_cast<float>(X_TILES));
                const float xRightScreen = WIDTH * ((x + 1) / static_cast<float>(X_TILES));
                for (size_t y = 0; y < Y_TILES; ++y)
                {
                    const float yTopScreen = HEIGHT * (y / static_cast<float>(Y_TILES));
                    const float yBottomScreen = HEIGHT * ((y + 1) / static_cast<float>(Y_TILES));

                    const glm::vec2 screenSpaceBottomLeft = glm::vec2(xLeftScreen, yBottomScreen);
                    const glm::vec2 screenSpaceTopRight = glm::vec2(xRightScreen, yTopScreen);

                    const glm::vec4 viewSpaceBottomLeft = util::math::screenToView(
                        screenSpaceBottomLeft, 
                        screenDimensions, 
                        inverseProjection);
                    const glm::vec4 viewSpaceTopRight = util::math::screenToView(
                        screenSpaceTopRight,
                        screenDimensions,
                        inverseProjection);

                    auto frontLeftBottom = glm::normalize(glm::vec3(viewSpaceBottomLeft.x, viewSpaceBottomLeft.y, -cameraNear));
                    auto frontRightBottom = glm::normalize(glm::vec3(viewSpaceTopRight.x, viewSpaceBottomLeft.y, -cameraNear));
                    auto frontLeftTop = glm::normalize(glm::vec3(viewSpaceBottomLeft.x, viewSpaceTopRight.y, -cameraNear));
                    auto frontRightTop = glm::normalize(glm::vec3(viewSpaceTopRight.x, viewSpaceTopRight.y, -cameraNear));

                    auto backLeftBottom = glm::normalize(glm::vec3(viewSpaceBottomLeft.x, viewSpaceBottomLeft.y, -cameraNear));
                    auto backRightBottom = glm::normalize(glm::vec3(viewSpaceTopRight.x, viewSpaceBottomLeft.y, -cameraNear));
                    auto backLeftTop = glm::normalize(glm::vec3(viewSpaceBottomLeft.x, viewSpaceTopRight.y, -cameraNear));
                    auto backRightTop = glm::normalize(glm::vec3(viewSpaceTopRight.x, viewSpaceTopRight.y, -cameraNear));

                    glm::vec3 frontLeftBottomIntersect,
                        frontRightBottomIntersect,
                        frontLeftTopIntersect,
                        frontRightTopIntersect,
                        backLeftBottomIntersect,
                        backRightBottomIntersect,
                        backLeftTopIntersect,
                        backRightTopIntersect;

                    util::math::Ray cameraRay{ mCamera->getWorldPosition(), frontLeftBottom };
                    util::math::rayPlaneIntersection(cameraRay, clusterNearPlane, frontLeftBottom);

                    cameraRay.direction = frontRightBottom;
                    util::math::rayPlaneIntersection(cameraRay, clusterNearPlane, frontRightBottom);

                    cameraRay.direction = frontLeftTop;
                    util::math::rayPlaneIntersection(cameraRay, clusterNearPlane, frontLeftTop);

                    cameraRay.direction = frontRightTop;
                    util::math::rayPlaneIntersection(cameraRay, clusterNearPlane, frontRightTop);

                    cameraRay.direction = backLeftBottom;
                    util::math::rayPlaneIntersection(cameraRay, clusterFarPlane, backLeftBottom);

                    cameraRay.direction = backRightBottom;
                    util::math::rayPlaneIntersection(cameraRay, clusterFarPlane, backRightBottom);

                    cameraRay.direction = backLeftTop;
                    util::math::rayPlaneIntersection(cameraRay, clusterFarPlane, backLeftTop);

                    cameraRay.direction = backRightTop;
                    util::math::rayPlaneIntersection(cameraRay, clusterFarPlane, backRightTop);

                    float minX, minY, maxX, maxY;
                    //util::math::AABB clus;

                    minX = std::min(frontLeftBottom.x, std::min(backLeftBottom.x, backLeftTop.x));
                    maxX = std::max(frontRightBottom.x, std::max(backRightBottom.x, backRightTop.x));
                    minY = std::min(frontLeftBottom.y, std::min(backLeftBottom.y, backRightBottom.y));
                    maxY = std::max(frontLeftTop.y, std::min(backLeftTop.y, backRightTop.y));

                    mLightClusters.push_back({ glm::vec3(minX, minY, nearSliceDepth), glm::vec3(maxX, maxY, farSliceDepth) });
                }
            }
        }
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
			&mPBRPassMap->view, 
			&mPBRDepthView, 
			&mPBRFramebuffer);
    }

    void UserApp::createShadowFramebuffer()
    {
        mShadowDepthMap = std::make_shared<TextureMap>(util::image::createImageMap(
            GpuManager::getDevice(),
            GpuManager::getAllocator(),
            GpuManager::getCommandPool(),
            GpuManager::getGraphicsQueue(),
            VK_FORMAT_D32_SFLOAT,
            2048,
            2048,
            0,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            1,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_VIEW_TYPE_2D,
            1,
            VK_IMAGE_ASPECT_DEPTH_BIT));

        util::framebuffer::createFramebuffer(
            GpuManager::getDevice(),
            mShadowRenderPass,
            VkExtent2D{ 2048, 2048 },
            nullptr,
            &mShadowDepthMap->view,
            &mShadowFramebuffer);

        // TODO: move this somewhere else once there's more than one light casting shadows
        mShadowSampleMap = std::make_shared<TextureMap>(util::image::createImageMap(
            GpuManager::getDevice(),
            GpuManager::getAllocator(),
            GpuManager::getCommandPool(),
            GpuManager::getGraphicsQueue(),
            VK_FORMAT_D32_SFLOAT,
            2048,
            2048,
            0,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            1,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_VIEW_TYPE_2D,
            1,
            VK_IMAGE_ASPECT_DEPTH_BIT));

		auto onetime = util::command::beginSingleTimeCommand(GpuManager::getDevice(), GpuManager::getCommandPool());
        util::image::transitionImageLayout(
			onetime,
			mShadowSampleMap->texture.memoryResource,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            0,
            1,
            0,
            VK_IMAGE_ASPECT_DEPTH_BIT);
		util::command::endSingleTimeCommand(
            GpuManager::getDevice(), 
            GpuManager::getCommandPool(), 
            onetime, 
            GpuManager::getGraphicsQueue());
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
            pbrPassDependencies,
            &colorAttachment,
            &depthAttachment);
    }

    void UserApp::createShadowRenderPass()
    {
        std::vector<VkAttachmentDescription> shadowpassAttachments = {
            util::renderpass::createDepthAttachment(
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        };

        VkAttachmentReference depthReference = {
			0,                                                  // attachment
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL    // layout
        };

        std::vector<VkSubpassDependency> shadowpassDependencies = { 
            util::renderpass::createSubpassDependency(
                VK_SUBPASS_EXTERNAL,
                0,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT),
            util::renderpass::createSubpassDependency(
                0,
                VK_SUBPASS_EXTERNAL,
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_DEPENDENCY_BY_REGION_BIT)
        };

        std::vector<VkSubpassDescription> shadowpassDescriptions = {
            util::renderpass::createSubpassDescription(
                std::vector<VkAttachmentReference>(),
                std::vector<VkAttachmentReference>(),
                &depthReference),
        };

        mShadowRenderPass = util::renderpass::createRenderPass(
            GpuManager::getDevice(),
            shadowpassDescriptions,
            shadowpassDependencies,
            shadowpassAttachments);
    }

    void UserApp::createFinalRenderPass()
    {
        auto finalColorAttachment = util::renderpass::createColorAttachment(mSwapchain.swapchainImageFormat);
		std::vector<VkSubpassDependency> finalPassDependencies = { util::renderpass::createSubpassDependency() };
        mFinalRenderPass = util::renderpass::createRenderPass(
            GpuManager::getDevice(),
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
				&mSwapchainViews[i], 
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

        // prepare shadow render pass
        VkRect2D shadowScissor = {
            {0, 0},
            VkExtent2D{2048, 2048}
        };

        VkViewport shadowViewport = {
            0.f,
            2048.f,
            2048.f,
            -2048.f,
            0.f,
            1.f
        };

        VkClearValue shadowClear;
        shadowClear.depthStencil = { 1.f, 0 };

        VkRenderPassBeginInfo shadowRenderBegin = {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            mShadowRenderPass,
            mShadowFramebuffer,
            shadowScissor,
            1,
            &shadowClear
        };

        std::vector<VkCommandBuffer> shadowCommandBuffers;
        auto shadowableGroup = mRegistry.group<>(entt::get<PBRMesh, ShadowBinding, WorldTransform>);
        auto lightCameraGroup = mRegistry.group<>(entt::get<WorldTransform, Projection, ShadowCaster>);
        //shadowCommandBuffers.reserve(lightCameraGroup.size());
        //lightCameraGroup.each([&](auto entity, const auto& transform, const auto& projection) {
        //    auto view = glm::inverse(transform.transform);
        //});
        for (const auto entity : lightCameraGroup)
        {
        //const auto& skyLightComponents = mRegistry.get<LightColor, Direction>(mSkyEntity);
			auto shadowInheritanceInfo = mApp->renderpassBegin(shadowRenderBegin);
            const auto& lightCamera = mRegistry.get<WorldTransform, Projection>(entity);
            auto& shadowMap = mRegistry.get<ShadowCaster>(entity);
            mApp->renderpassExecuteAndClose(
                {
					mShadowRenderer->drawElements(
						shadowInheritanceInfo,
						shadowViewport,
						shadowScissor,
						lightCamera,
						shadowableGroup)
				}
            );
            // copy shadow map from framebuffer to texture image
            util::image::framebufferImageToTexture(
                mApp->getPrimaryCommandBuffer(),
                *mShadowDepthMap,
                shadowMap.shadowMap);
        }

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
        auto pbrGroup = mRegistry.group<>(entt::get<PBRMesh, PBRBinding, WorldTransform>);
        auto lightGroup = mRegistry.group<>(entt::get<LightColor, LightAttenuation, WorldTransform>, entt::exclude<SpotLight>);
        auto spotlightGroup = mRegistry.group<>(entt::get<LightColor, LightAttenuation, SpotLight, WorldTransform>);
        const auto& skyLightComponents = mRegistry.get<LightColor, Direction>(mSkyEntity);
        auto shadowmapView = mRegistry.view<ShadowCaster>();
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
            skyLightComponents,
            shadowmapView));

        auto debugGroup = mRegistry.group<DebugDrawMesh, DebugDrawBinding>(entt::get<WorldTransform>);
        pbrCommandBuffers.push_back(mDebugRenderer->drawElements(
            pbrInheritanceInfo,
            viewport,
            scissor,
            *mCamera,
            debugGroup));

        if (pbrCommandBuffers.size())
        {
			mApp->renderpassExecuteAndClose(pbrCommandBuffers);
        }

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
