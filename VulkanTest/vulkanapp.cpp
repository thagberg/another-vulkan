#include "stdafx.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <limits>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_SWIZZLE_XYZW
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tiny_gltf.h"
#include "imgui/imgui.h"

#include "vulkanapp.h"
#include "vulkan-util.h"
#include "RenderObject.h"
#include "InputManager.h"
#include "StaticMesh.h"
#include "ResourceManager.h"

hvk::VulkanApp* currentApp;

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

tinygltf::TinyGLTF modelLoader = tinygltf::TinyGLTF();

namespace hvk {

	void processGltfNode(
		tinygltf::Node& node, 
		tinygltf::Model& model, 
		std::vector<Vertex, Hallocator<Vertex>>& vertices, 
		std::vector<VertIndex>& indices, 
		Material& mat)
	{
		if (node.mesh >= 0) {
			tinygltf::Mesh mesh = model.meshes[node.mesh];
			for (size_t j = 0; j < mesh.primitives.size(); ++j) {
				tinygltf::Primitive prim = mesh.primitives[j];
				const auto positionAttr = prim.attributes.find("POSITION");
				const auto uvAttr = prim.attributes.find("TEXCOORD_0");
				const auto normalAttr = prim.attributes.find("NORMAL");

				assert(positionAttr != prim.attributes.end());
				assert(uvAttr != prim.attributes.end());
				assert(normalAttr != prim.attributes.end());

				// process position data
				const tinygltf::Accessor positionAccess = model.accessors[positionAttr->second];
				assert(positionAccess.type == TINYGLTF_TYPE_VEC3);
				const size_t numPositions = positionAccess.count;
				const tinygltf::BufferView positionView = model.bufferViews[positionAccess.bufferView];
				const float* positionData = reinterpret_cast<const float*>(
					&(model.buffers[positionView.buffer].data[positionView.byteOffset + positionAccess.byteOffset]));

				// process UV data
				const tinygltf::Accessor uvAccess = model.accessors[uvAttr->second];
				assert(uvAccess.type == TINYGLTF_TYPE_VEC2);
				const tinygltf::BufferView uvView = model.bufferViews[uvAccess.bufferView];
				const float* uvData = reinterpret_cast<const float*>(
					&(model.buffers[uvView.buffer].data[uvView.byteOffset + uvAccess.byteOffset]));

				// process normal data
				const tinygltf::Accessor normalAccess = model.accessors[normalAttr->second];
				assert(normalAccess.type == TINYGLTF_TYPE_VEC3);
				const tinygltf::BufferView normalView = model.bufferViews[normalAccess.bufferView];
				const float* normalData = reinterpret_cast<const float*>(
					&(model.buffers[normalView.buffer].data[normalView.byteOffset + normalAccess.byteOffset]));

				assert(positionAccess.count == uvAccess.count);

				//RenderObject::allocateVertices(numPositions, vertices);
				vertices.reserve(numPositions);

				for (size_t k = 0; k < numPositions; ++k) {
					Vertex v{};
					v.pos = glm::make_vec3(&positionData[k * 3]);
					v.normal = glm::make_vec3(&normalData[k * 3]);
					v.texCoord = glm::make_vec2(&uvData[k * 2]);
					vertices.push_back(v);
				}

				// process indices
				const tinygltf::Accessor indexAccess = model.accessors[prim.indices];
				assert(indexAccess.type == TINYGLTF_TYPE_SCALAR);
				const size_t numIndices = indexAccess.count;
				const tinygltf::BufferView indexView = model.bufferViews[indexAccess.bufferView];
				// TODO: need to handle more than just uint16_t indices (unsigned short)
				const uint16_t* indexData = reinterpret_cast<const uint16_t*>(
					&(model.buffers[indexView.buffer].data[indexView.byteOffset + indexAccess.byteOffset]));

				//RenderObject::allocateIndices(numIndices, indices);
				indices.reserve(numIndices);

				for (size_t k = 0; k < numIndices; ++k) {
					indices.push_back(indexData[k]);
				}

				// process material
				//mat.diffuseProp.texture = Pool<tinygltf::Image>::alloc(model.images[model.materials[prim.material].pbrMetallicRoughness.baseColorTexture.index]);
				mat.diffuseProp.texture = new tinygltf::Image(model.images[model.materials[prim.material].pbrMetallicRoughness.baseColorTexture.index]);
			}
		}

		for (const int& childId : node.children) {
			tinygltf::Node& childNode = model.nodes[childId];
			processGltfNode(childNode, model, vertices, indices, mat);
		}
	}

	//std::unique_ptr<StaticMesh> createMeshFromGltf(const std::string& filename) 
	auto createMeshFromGltf(const std::string& filename) 
	{
		// TODO: use a real allocation system instead of "new"
		//std::vector<Vertex>* vertices = new std::vector<Vertex>();
		std::shared_ptr<std::vector<Vertex, Hallocator<Vertex>>> vertices = std::make_shared<std::vector<Vertex, Hallocator<Vertex>>>();
		//std::vector<VertIndex>* indices = new std::vector<VertIndex>();
		std::shared_ptr<std::vector<VertIndex>> indices = std::make_shared<std::vector<VertIndex>>();
		std::shared_ptr<Material> mat = std::make_shared<Material>();
		//Material* mat = Pool<Material>::alloc();

		tinygltf::Model model;
		std::string err, warn;

		bool modelLoaded = modelLoader.LoadASCIIFromFile(&model, &err, &warn, filename);
		if (!err.empty()) {
			std::cout << err << std::endl;
		}
		if (!warn.empty()) {
			std::cout << warn << std::endl;
		}
		assert(modelLoaded);

		const tinygltf::Scene modelScene = model.scenes[model.defaultScene];
		for (const int& nodeId : modelScene.nodes) {
			tinygltf::Node& sceneNode = model.nodes[nodeId];
			processGltfNode(sceneNode, model, *vertices.get(), *indices.get(), *mat.get());
		}

		// TODO: this is going to result in a memory leak!!!  See above comment
		//return StaticMesh(*vertices, *indices, *mat);
		return Pool<StaticMesh>::alloc(vertices, indices, mat);
	}


    VulkanApp::VulkanApp(int width, int height, const char* windowTitle) :
        mWindowWidth(width),
        mWindowHeight(height),
        mInstance(VK_NULL_HANDLE),
        mPhysicalDevice(VK_NULL_HANDLE),
        mDevice(VK_NULL_HANDLE),
        //mPipelineLayout(VK_NULL_HANDLE),
        //mGraphicsPipeline(VK_NULL_HANDLE),
        mWindow(hvk::initializeWindow(width, height, windowTitle), glfwDestroyWindow),
		//mRenderer(),
		mMeshRenderer(nullptr),
		mUiRenderer(nullptr),
		mDebugRenderer(nullptr),
		mLastX(0.f),
		mLastY(0.f),
		mMouseLeftDown(false),
		mClock(),
		mCameraController(nullptr),
		mObjectNode(nullptr),
		mCameraNode(nullptr),
		mLightNode(nullptr)
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

		cleanupSwapchain();
	
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
		// create allocator
		VmaAllocatorCreateInfo allocatorCreate = {};
		allocatorCreate.physicalDevice = mPhysicalDevice;
		allocatorCreate.device = mDevice;
		vmaCreateAllocator(&allocatorCreate, &mAllocator);

        vkGetDeviceQueue(mDevice, mGraphicsIndex, 0, &mGraphicsQueue);
        mCommandPool = createCommandPool(mDevice, mGraphicsIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

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

		initFramebuffers();

		// create Camera object
		mCameraNode = std::make_shared<Camera>(
			45.0f,
			mSwapchain.swapchainExtent.width / (float)mSwapchain.swapchainExtent.height,
			0.1f,
			1000.0f,
			nullptr,
			glm::mat4(1.0f));
		mCameraController = CameraController(mCameraNode);
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

		//mRenderer.init(device, mAllocator, mGraphicsQueue, mRenderPass, mCameraNode, mSwapchain.swapchainImageFormat, mSwapchain.swapchainExtent);
		mMeshRenderer = std::make_shared<StaticMeshGenerator>(device, mAllocator, mGraphicsQueue, mRenderPass, mCommandPool);

		/*
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = mSwapchain.swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        //colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = VK_FORMAT_D32_SFLOAT;  // TODO: make this dynamic
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		//depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkRenderPass uiRenderPass = createRenderPass(mDevice, mSwapchain.swapchainImageFormat, colorAttachment, depthAttachment);
		*/
		mUiRenderer = std::make_shared<UiDrawGenerator>(device, mAllocator, mGraphicsQueue, mRenderPass, mCommandPool, mSwapchain.swapchainExtent);

		mDebugRenderer = std::make_shared<DebugDrawGenerator>(device, mAllocator, mGraphicsQueue, mRenderPass, mCommandPool);

		glm::mat4 modelTransform = glm::mat4(1.0f);
		modelTransform = glm::scale(modelTransform, glm::vec3(0.01f, 0.01f, 0.01f));
		std::shared_ptr<StaticMesh> duckMesh(std::move(createMeshFromGltf("resources/duck/Duck.gltf")));
		std::shared_ptr<StaticMeshRenderObject> duckObj = std::make_shared<StaticMeshRenderObject>(nullptr, modelTransform, duckMesh);
		//RenderObjRef modelObj = RenderObject::createFromGltf("resources/duck/Duck.gltf", nullptr, modelTransform);
		//mRenderer.addRenderable(std::static_pointer_cast<RenderObject>(duckObj));
		mMeshRenderer->addStaticMeshObject(duckObj);

        mObjectNode = std::make_shared<Node>(nullptr, glm::mat4(1.0f));
		glm::mat4 lightTransform = glm::mat4(1.0f);
		lightTransform = glm::scale(lightTransform, glm::vec3(0.1f));
		lightTransform = glm::translate(lightTransform, glm::vec3(3.f, 2.f, 1.5f));
		mLightNode = std::make_shared<Light>(
			nullptr, 
			lightTransform,
			glm::vec3(1.f, 1.f, 1.f),
            0.3f);
		//mRenderer.addLight(mLightNode);
		mMeshRenderer->addLight(mLightNode);

		DebugMesh::ColorVertices lightVertices = std::make_shared<std::vector<ColorVertex>>();
		lightVertices->reserve(8);
		lightVertices->push_back(ColorVertex{
			glm::vec3(0.f, 0.f, 0.f),
			glm::vec3(1.f, 1.f, 1.f)});
		lightVertices->push_back(ColorVertex{
			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(1.f, 1.f, 1.f)});
		lightVertices->push_back(ColorVertex{
			glm::vec3(0.f, 1.f, 0.f),
			glm::vec3(1.f, 1.f, 1.f)});
		lightVertices->push_back(ColorVertex{
			glm::vec3(1.f, 1.f, 0.f),
			glm::vec3(1.f, 1.f, 1.f)});
		lightVertices->push_back(ColorVertex{
			glm::vec3(0.f, 0.f, 1.f),
			glm::vec3(1.f, 1.f, 1.f)});
		lightVertices->push_back(ColorVertex{
			glm::vec3(1.f, 0.f, 1.f),
			glm::vec3(1.f, 1.f, 1.f)});
		lightVertices->push_back(ColorVertex{
			glm::vec3(0.f, 1.f, 1.f),
			glm::vec3(1.f, 1.f, 1.f)});
		lightVertices->push_back(ColorVertex{
			glm::vec3(1.f, 1.f, 1.f),
			glm::vec3(1.f, 1.f, 1.f)});
		std::shared_ptr<std::vector<VertIndex>> lightIndices = std::make_shared<std::vector<VertIndex>>();
		lightIndices->reserve(24);
		lightIndices->push_back(0);
		lightIndices->push_back(1);
		lightIndices->push_back(2);

		lightIndices->push_back(2);
		lightIndices->push_back(1);
		lightIndices->push_back(3);

		lightIndices->push_back(3);
		lightIndices->push_back(7);
		lightIndices->push_back(2);

		lightIndices->push_back(2);
		lightIndices->push_back(6);
		lightIndices->push_back(7);

		lightIndices->push_back(7);
		lightIndices->push_back(4);
		lightIndices->push_back(5);

		lightIndices->push_back(7);
		lightIndices->push_back(6);
		lightIndices->push_back(4);

		lightIndices->push_back(5);
		lightIndices->push_back(0);
		lightIndices->push_back(1);
		std::shared_ptr<DebugMesh> debugMesh = std::make_shared<DebugMesh>(lightVertices, lightIndices);
		auto lightBox = std::make_shared<DebugMeshRenderObject>(
			nullptr,
			//glm::translate(glm::mat4(1.0f), glm::vec3(5.f, 0.f, 0.f)), 
			mLightNode->getTransform(), 
			debugMesh);
		mDebugRenderer->addDebugMeshObject(lightBox);

        mImageAvailable = createSemaphore(mDevice);
    }

	void VulkanApp::recreateSwapchain() {
		vkDeviceWaitIdle(mDevice);
		//mRenderer.invalidateRenderer();
		mMeshRenderer->invalidate();
		mUiRenderer->invalidate();
		mDebugRenderer->invalidate();
		cleanupSwapchain();
		glfwGetFramebufferSize(mWindow.get(), &mWindowWidth, &mWindowHeight);

        if (createSwapchain(mPhysicalDevice, mDevice, mSurface, mWindowWidth, mWindowHeight, mSwapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Swapchain");
        }

		initFramebuffers();
		mCameraNode->updateProjection(
			45.0f,
			mSwapchain.swapchainExtent.width / (float)mSwapchain.swapchainExtent.height,
			0.1f,
			1000.0f);
		//mRenderer.updateRenderPass(mRenderPass, mSwapchain.swapchainExtent);
		mMeshRenderer->updateRenderPass(mRenderPass);
		mUiRenderer->updateRenderPass(mRenderPass, mSwapchain.swapchainExtent);
		mDebugRenderer->updateRenderPass(mRenderPass);
	}

	void VulkanApp::initializeApp() {
		glfwSetWindowUserPointer(mWindow.get(), this);
		glfwSetFramebufferSizeCallback(mWindow.get(), VulkanApp::handleWindowResize);
	}

    void VulkanApp::init() {
        try {
			ResourceManager::initialize(200 * 1000 * 1000);
			ImGui::CreateContext();
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
			0.3f
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
			*mCameraNode.get(),
			ambient);

		commandBuffers[1] = mDebugRenderer->drawFrame(
			inheritanceInfo, 
			mFramebuffers[imageIndex], 
			viewport, 
			scissor,
			*mCameraNode.get());

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

    void VulkanApp::run() {
		InputManager::init(mWindow);
		ImGuiIO& io = ImGui::GetIO();

		double frameTime = mClock.getDelta();
		bool cameraDrag = false;
        while (!glfwWindowShouldClose(mWindow.get())) {
			mClock.start();
			frameTime = mClock.getDelta();

			//ImGui::Che

			// TODO: figure out how to poll GLFW events outside of InputManager
			//	while still capturing current vs previous mouse state correctly
			InputManager::update();
			MouseState mouse = InputManager::currentMouseState;
			MouseState prevMouse = InputManager::previousMouseState;
			io.DeltaTime = static_cast<float>(frameTime);
			io.MousePos = ImVec2(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
			io.MouseDown[0] = mouse.leftDown;
            io.KeyCtrl = InputManager::isPressed(GLFW_KEY_LEFT_CONTROL);

			if (InputManager::currentKeysPressed[GLFW_KEY_ESCAPE]) {
				glfwSetWindowShouldClose(mWindow.get(), GLFW_TRUE);
			}
			if (InputManager::currentKeysPressed[GLFW_KEY_Y] && !InputManager::previousKeysPressed[GLFW_KEY_Y]) {
				// TODO: add normals renderer
				//Renderer::setDrawNormals(!Renderer::getDrawNormals());
			}

			bool mouseClicked = mouse.leftDown && !prevMouse.leftDown;
			bool mouseReleased = prevMouse.leftDown && !mouse.leftDown;
			if (mouseClicked && !io.WantCaptureMouse) {
				cameraDrag = true;
				glfwSetInputMode(mWindow.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			if (mouseReleased) {
				cameraDrag = false;
				glfwSetInputMode(mWindow.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}

			/*
			if (cameraDrag) {
				float sensitivity = 0.1f;
				cameraRotation.pitch = (mouse.y - prevMouse.y) * sensitivity;
				cameraRotation.yaw = (prevMouse.x - mouse.x) * sensitivity;
			}
			*/

			float sensitivity = 0.1f;
			float mouseDeltY = static_cast<float>(mouse.y - prevMouse.y);
			float mouseDeltX = static_cast<float>(prevMouse.x - mouse.x);

			// camera updates
			//updateCamera(frameTime);
			std::vector<Command> cameraCommands;
			cameraCommands.reserve(6);
			if (InputManager::isPressed(GLFW_KEY_W)) {
				cameraCommands.push_back({ 0, "move_forward", -1.0f });
			}
			if (InputManager::isPressed(GLFW_KEY_S)) {
				cameraCommands.push_back({ 0, "move_forward", 1.0f });
			}
			if (InputManager::isPressed(GLFW_KEY_A)) {
				cameraCommands.push_back({ 1, "move_right", -1.0f });
			}
			if (InputManager::isPressed(GLFW_KEY_D)) {
				cameraCommands.push_back({ 1, "move_right", 1.0f });
			}
			if (InputManager::isPressed(GLFW_KEY_Q)) {
				cameraCommands.push_back({ 2, "move_up", -1.0f });
			}
			if (InputManager::isPressed(GLFW_KEY_E)) {
				cameraCommands.push_back({ 2, "move_up", 1.0f });
			}
			if (InputManager::isPressed(GLFW_KEY_UP))
			{
				cameraCommands.push_back({ 3, "camera_pitch", 0.25f });
			}
			if (InputManager::isPressed(GLFW_KEY_DOWN))
			{
				
				cameraCommands.push_back({ 3, "camera_pitch", -0.25f });
			}
			if (InputManager::isPressed(GLFW_KEY_LEFT))
			{
				
				cameraCommands.push_back({ 4, "camera_yaw", -0.25f });
			}
			if (InputManager::isPressed(GLFW_KEY_RIGHT))
			{
				cameraCommands.push_back({ 4, "camera_yaw", 0.25f });
			}
			if (cameraDrag) {
				if (mouseDeltY) {
					cameraCommands.push_back({ 3, "camera_pitch", mouseDeltY * sensitivity });
				}
				if (mouseDeltX) {
					cameraCommands.push_back({ 4, "camera_yaw", mouseDeltX * sensitivity });
				}
			}
			mCameraController.update(frameTime, cameraCommands);

        	/**************
			Do UI Stuff Here
        	***************/
			ImGui::NewFrame();
			ImGui::Begin("Lights");
			ImGui::Text("Dynamic Lights");
			glm::vec3 col = mLightNode->getColor();
			float intensity = mLightNode->getIntensity();
			ImGui::ColorEdit3("Light Color", &col.x);
			mLightNode->setColor(col);
			ImGui::SliderFloat("Light Intensity", &intensity, 0.f, 1.f);
			mLightNode->setIntensity(intensity);
			glm::vec3 lightPos = mLightNode->getLocalPosition();
            float posMagnitude = glm::length(lightPos);
            ImGui::DragFloat3("Light Position", &lightPos.x, 0.01f);
            glm::vec3 posDiff = lightPos - mLightNode->getLocalPosition();
            mLightNode->translateLocal(posDiff);
			ImGui::End();
			ImGui::ShowDemoWindow();
			ImGui::EndFrame();

            drawFrame();
			mClock.end();
        }

        vkDeviceWaitIdle(mDevice);
    }

	void VulkanApp::handleWindowResize(GLFWwindow* window, int width, int height) {
		VulkanApp* thisApp = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
		thisApp->recreateSwapchain();
	}
}