#include "stdafx.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "vulkanapp.h"

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<hvk::Vertex> vertices = {
	{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, 
	{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, 
};

namespace hvk {

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type on Physical Device");
	}

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

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* pDebugMesenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, nullptr, pDebugMesenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
		SwapchainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if (formatCount) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		if (presentModeCount) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& presentMode : availablePresentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return presentMode;
			} else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				bestMode = presentMode;
			}
		}

		return bestMode;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			VkExtent2D extent = { width, height };

			extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
			extent.width = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

			return extent;
		}
	}

	VkResult createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, int width, int height, hvk::Swapchain& swapchain) {
		SwapchainSupportDetails support = querySwapchainSupport(physicalDevice, surface);
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
		VkExtent2D extent = chooseSwapExtent(support.capabilities, width, height);

		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount && support.capabilities.maxImageCount < imageCount) {
			imageCount = support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		// relying on graphics and present queues being the same
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = VK_NULL_HANDLE;
		createInfo.preTransform = support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		swapchain.swapchainImageFormat = surfaceFormat.format;
		swapchain.swapchainExtent = extent;

		return vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain.swapchain);
	}

	VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module");
		}

		return shaderModule;
	}

	VkRenderPass createRenderPass(VkDevice device, VkFormat swapchainImageFormat) {
		VkRenderPass renderPass;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &colorAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render pass");
		}

		return renderPass;
	}

	VkPipelineLayout createGraphicsPipelineLayout( VkDevice device) {
		VkPipelineLayout pipelineLayout;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout");
		}

		return pipelineLayout;
	}

	VkPipeline createGraphicsPipeline(
		VkDevice device, 
		VkExtent2D swapchainExtent, 
		VkRenderPass renderPass, 
		VkPipelineLayout& pipelineLayout) {

		VkPipeline graphicsPipeline;

		auto vertShaderCode = readFile("shaders/compiled/vert.spv");
		auto fragShaderCode = readFile("shaders/compiled/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

		VkPipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertStageInfo.module = vertShaderModule;
		vertStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageInfo.module = fragShaderModule;
		fragStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

		auto bindingDescription = hvk::Vertex::getBindingDescription();
		auto attributeDescriptions = hvk::Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchainExtent.width;
		viewport.height = (float)swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Graphics Pipeline");
		}

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);

		return graphicsPipeline;
	}

	void createFramebuffers(
		VkDevice device, 
		hvk::SwapchainImageViews& imageViews, 
		VkRenderPass renderPass, 
		VkExtent2D extent, 
		hvk::FrameBuffers& oFramebuffers) {

		oFramebuffers.resize(imageViews.size());

		for (size_t i = 0; i < imageViews.size(); i++) {
			VkImageView attachments[] = {
				imageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = extent.width;
			framebufferInfo.height = extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &oFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Framebuffer");
			}
		}
	}

	VkCommandPool createCommandPool(VkDevice device, int queueFamilyIndex) {
		VkCommandPool commandPool;

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = 0;

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Command Pool");
		}

		return commandPool;
	}

	VkBuffer createVertexBuffer(VkDevice device, const std::vector<hvk::Vertex>& vertices) {
		VkBuffer vertexBuffer;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(vertices[0]) * vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vertex Buffer");
		}

		return vertexBuffer;
	}

	VkDeviceMemory allocateVertexBufferMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer vertexBuffer) {
		VkDeviceMemory vertexBufferMemory;

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
		uint32_t memoryType = findMemoryType(
			memRequirements.memoryTypeBits, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			physicalDevice);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memoryType;

		if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate memory for vertex buffer");
		}

		return vertexBufferMemory;

	}

	void createCommandBuffers(
		VkDevice device, 
		VkCommandPool commandPool, 
		VkRenderPass renderPass,
		VkExtent2D swapchainExtent,
		VkPipeline graphicsPipeline,
		hvk::FrameBuffers& frameBuffers, 
		hvk::CommandBuffers& oCommandBuffers) {

		oCommandBuffers.resize(frameBuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)oCommandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, oCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Command Buffers");
		}

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		for (size_t i = 0; i < oCommandBuffers.size(); i++) {
			VkCommandBuffer thisCommandBuffer = oCommandBuffers[i];
			VkFramebuffer thisFramebuffer = frameBuffers[i];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(thisCommandBuffer, &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("Failed to begin recording Command Buffer");
			}

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = thisFramebuffer;
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapchainExtent;
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(thisCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(thisCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			vkCmdDraw(thisCommandBuffer, 3, 1, 0, 0);
			vkCmdEndRenderPass(thisCommandBuffer);

			if (vkEndCommandBuffer(thisCommandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("Failed to record Command Buffer");
			}
		}
	}

	VkSemaphore createSemaphore(VkDevice device) {
		VkSemaphore semaphore;

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Semaphore");
		}

		return semaphore;
	}

	VulkanApp::VulkanApp(int width, int height, const char* windowTitle) :
		mWindowWidth(width),
		mWindowHeight(height),
		mInstance(VK_NULL_HANDLE),
		mPhysicalDevice(VK_NULL_HANDLE),
		mDevice(VK_NULL_HANDLE),
		mPipelineLayout(VK_NULL_HANDLE),
		mGraphicsPipeline(VK_NULL_HANDLE),
		mWindow(initializeWindow(width, height, windowTitle), glfwDestroyWindow)
	{

	}

	VulkanApp::~VulkanApp() {
		glfwTerminate();
		vkDestroySemaphore(mDevice, mImageAvailable, nullptr);
		vkDestroySemaphore(mDevice, mRenderFinished, nullptr);
		vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
		vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		for (auto framebuffer : mFramebuffers) {
			vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
		}
		for (auto imageView : mImageViews) {
			vkDestroyImageView(mDevice, imageView, nullptr);
		}
		vkDestroySwapchainKHR(mDevice, mSwapchain.swapchain, nullptr);
		vkDestroyBuffer(mDevice, mVertexBuffer, nullptr);
		vkFreeMemory(mDevice, mVertexBufferMemory, nullptr);
		vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
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
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());
		mPhysicalDevice = devices[0];

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

		if (createSwapchain(mPhysicalDevice, mDevice, mSurface, mWindowWidth, mWindowHeight, mSwapchain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Swapchain");
		}

		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, nullptr);
		mSwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(mDevice, mSwapchain.swapchain, &imageCount, mSwapchainImages.data());

		mImageViews.resize(mSwapchainImages.size());
		for (size_t i = 0; i < mSwapchainImages.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = mSwapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = mSwapchain.swapchainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(mDevice, &createInfo, nullptr, &mImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Swapchain Image Views");
			}
		}

		mRenderPass = createRenderPass(mDevice, mSwapchain.swapchainImageFormat);

		mPipelineLayout = createGraphicsPipelineLayout(mDevice);

		mGraphicsPipeline = createGraphicsPipeline(mDevice, mSwapchain.swapchainExtent, mRenderPass, mPipelineLayout);

		createFramebuffers(mDevice, mImageViews, mRenderPass, mSwapchain.swapchainExtent, mFramebuffers);

		mCommandPool = createCommandPool(mDevice, mGraphicsIndex);

		mVertexBuffer = createVertexBuffer(mDevice, vertices);

		mVertexBufferMemory = allocateVertexBufferMemory(mDevice, mPhysicalDevice, mVertexBuffer);

		vkBindBufferMemory(mDevice, mVertexBuffer, mVertexBufferMemory, 0);

		createCommandBuffers(mDevice, mCommandPool, mRenderPass, mSwapchain.swapchainExtent, mGraphicsPipeline, mFramebuffers, mCommandBuffers);

		mImageAvailable = createSemaphore(mDevice);
		mRenderFinished = createSemaphore(mDevice);
	}

	void VulkanApp::init() {
		initializeVulkan();
		enableVulkanValidationLayers();
		initializeDevice();
		initializeRenderer();
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

		VkSemaphore waitSemaphores[] = { mImageAvailable };
		VkSemaphore signalSemaphores[] = { mRenderFinished };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command to Command Buffer");
		}
		
		VkSwapchainKHR swapchains[] = { mSwapchain.swapchain };
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		vkQueuePresentKHR(mGraphicsQueue, &presentInfo);
	}

	void VulkanApp::run() {
		while (!glfwWindowShouldClose(mWindow.get())) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(mDevice);
	}
}