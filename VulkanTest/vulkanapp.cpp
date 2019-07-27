#include "stdafx.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkanapp.h"


#define BYTES_PER_PIXEL 4

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<hvk::Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}, 
	{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, 
	{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, 
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
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
		//colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

	VkPipelineLayout createGraphicsPipelineLayout(VkDevice device, VkDescriptorSetLayout layout) {
		VkPipelineLayout pipelineLayout;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &layout;
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
		//rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
		std::vector<VkBuffer>& vertexBuffers,
		std::vector<VkBuffer>& indexBuffers,
		hvk::CommandBuffers& oCommandBuffers,
		VkPipelineLayout pipelineLayout,
		DescriptorSets& descriptorSets) {

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

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(thisCommandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), offsets);
			vkCmdBindIndexBuffer(thisCommandBuffer, indexBuffers[0], 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(
				thisCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
			// TODO: don't use global vertices vector size
			//vkCmdDraw(thisCommandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			vkCmdDrawIndexed(thisCommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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

	VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device) {
		VkDescriptorSetLayout descriptorSetLayout;

		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Descriptor Set Layout");
		}

		return descriptorSetLayout;
	}

	VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t descriptorCount, uint32_t numSets) {
		VkDescriptorPool descriptorPool;

		VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
		poolSize.descriptorCount = descriptorCount;

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = numSets;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Descriptor Pool");
		}

		return descriptorPool;
	}

	DescriptorSets createDescriptorSets(VkDevice device, VkDescriptorPool pool, std::vector<VkDescriptorSetLayout>& layouts) {
		DescriptorSets descriptorSets(layouts.size());

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Descriptor Sets");
		}

		return descriptorSets;
	}

	VkCommandBuffer beginSingleTimeCommand(VkDevice device, VkCommandPool commandPool) {
		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void endSingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void transitionImageLayout(
		VkDevice device, 
		VkCommandPool commandPool, 
		VkQueue graphicsQueue, 
		VkImage image, 
		VkFormat format, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout) {

		VkCommandBuffer commandBuffer = beginSingleTimeCommand(device, commandPool);

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("Unsupported layout transition");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		endSingleTimeCommand(device, commandPool, commandBuffer, graphicsQueue);
	}

	void copyBufferToImage(
		VkDevice device, 
		VkCommandPool commandPool, 
		VkQueue graphicsQueue, 
		VkBuffer buffer, 
		VkImage image, 
		uint32_t width, 
		uint32_t height) {

		VkCommandBuffer commandBuffer = beginSingleTimeCommand(device, commandPool);

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommand(device, commandPool, commandBuffer, graphicsQueue);
	}

	hvk::Resource<VkImage> createTextureImage(
		VkDevice device, 
		VmaAllocator allocator, 
		VkCommandPool commandPool, 
		VkQueue graphicsQueue) {

		hvk::Resource<VkImage> textureResource;

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load("resources/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			throw std::runtime_error("Failed to load Texture Image");
		}

		VkDeviceSize imageSize = texWidth * texHeight * BYTES_PER_PIXEL;

		// copy image data into a staging buffer which will then be used
		// to transfer to a Vulkan image
		VkBuffer imageStagingBuffer;
		VkBufferCreateInfo stagingCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		stagingCreateInfo.size = imageSize;
		stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocation stagingAllocation;
		VmaAllocationInfo stagingAllocationInfo;
		VmaAllocationCreateInfo stagingAllocCreateInfo = {};
		stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		
		vmaCreateBuffer(
			allocator, 
			&stagingCreateInfo, 
			&stagingAllocCreateInfo, 
			&imageStagingBuffer, 
			&stagingAllocation, 
			&stagingAllocationInfo);
	
		void* stagingData;
		vmaMapMemory(allocator, stagingAllocation, &stagingData);
		memcpy(stagingData, pixels, imageSize);
		vmaUnmapMemory(allocator, stagingAllocation);

		// free the pixel data
		stbi_image_free(pixels);

		VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(texWidth);
		imageInfo.extent.height = static_cast<uint32_t>(texHeight);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;

		VmaAllocationCreateInfo imageAllocationCreateInfo = {};
		imageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vmaCreateImage(
			allocator, 
			&imageInfo, 
			&imageAllocationCreateInfo, 
			&textureResource.memoryResource, 
			&textureResource.allocation, 
			&textureResource.allocationInfo);

		transitionImageLayout(
			device, 
			commandPool, 
			graphicsQueue, 
			textureResource.memoryResource, 
			VK_FORMAT_R8G8B8A8_UNORM, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		copyBufferToImage(
			device, 
			commandPool, 
			graphicsQueue, 
			imageStagingBuffer, 
			textureResource.memoryResource, 
			texWidth, 
			texHeight);

		transitionImageLayout(
			device, 
			commandPool, 
			graphicsQueue, 
			textureResource.memoryResource, 
			VK_FORMAT_R8G8B8A8_UNORM, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vmaDestroyBuffer(allocator, imageStagingBuffer, stagingAllocation);

		return textureResource;
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
		vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
		vkDestroyBuffer(mDevice, mVertexBuffer, nullptr);
		/*for (int i = 0; i < mUniformBuffers.size(); i++) {
			auto ubo = mUniformBuffers[i];
			auto allocInfo = mUniformAllocations[i];
			vmaDestroyBuffer(mAllocator, ubo, allocInfo);
		}*/
		// TODO: FREE VMA BUFFER MEMORY
		//vmaFreeMemory(mAllocator, )
		//vkFreeMemory(mDevice, mVertexBufferMemory, nullptr);
		//vkFreeMemory(mDevice, mIndexbu, nullptr);
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
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

	void VulkanApp::initializeAllocator() {
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = mPhysicalDevice;
		allocatorInfo.device = mDevice;
		vmaCreateAllocator(&allocatorInfo, &mAllocator);
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

		mDescriptorSetLayout = createDescriptorSetLayout(mDevice);

		mDescriptorPool = createDescriptorPool(
			mDevice, static_cast<uint32_t>(mSwapchainImages.size()), static_cast<uint32_t>(mSwapchainImages.size()));

		std::vector<VkDescriptorSetLayout> descriptorLayoutCopies(mSwapchainImages.size(), mDescriptorSetLayout);
		mDescriptorSets = createDescriptorSets(mDevice, mDescriptorPool, descriptorLayoutCopies);

		// create UBOs
		uint32_t uboMemorySize = sizeof(hvk::UniformBufferObject);
		VkBufferCreateInfo uboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		uboInfo.size = uboMemorySize;
		uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		mUniformBuffers.resize(mSwapchainImages.size());
		mUniformAllocations.resize(mSwapchainImages.size());

		for (int i = 0; i < mSwapchainImages.size(); i++) {
			VmaAllocation uniformAllocation;
			VmaAllocationInfo uniformAllocInfo;
			VmaAllocationCreateInfo uniformAllocCreateInfo = {};
			uniformAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			uniformAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
			vmaCreateBuffer(mAllocator, &uboInfo, &uniformAllocCreateInfo, &mUniformBuffers[i], &uniformAllocation, &uniformAllocInfo);
			mUniformAllocations[i] = uniformAllocInfo;
		}

		// populate descriptor sets
		for (size_t i = 0; i < descriptorLayoutCopies.size(); i++) {
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = mUniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(hvk::UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrite.dstSet = mDescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(mDevice, 1, &descriptorWrite, 0, nullptr);
		}

		mPipelineLayout = createGraphicsPipelineLayout(mDevice, mDescriptorSetLayout);

		mGraphicsPipeline = createGraphicsPipeline(mDevice, mSwapchain.swapchainExtent, mRenderPass, mPipelineLayout);

		createFramebuffers(mDevice, mImageViews, mRenderPass, mSwapchain.swapchainExtent, mFramebuffers);

		mCommandPool = createCommandPool(mDevice, mGraphicsIndex);

		uint32_t vertexMemorySize = sizeof(vertices[0]) * vertices.size();
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = vertexMemorySize;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		//allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
			mAllocator, &bufferInfo, &allocCreateInfo, &mVertexBuffer, &mVertexAllocation, &mVertexAllocationInfo);

		memcpy(mVertexAllocationInfo.pMappedData, vertices.data(), (size_t)vertexMemorySize);

		uint32_t indexMemorySize = sizeof(uint16_t) * indices.size();
		VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		iboInfo.size = indexMemorySize;
		iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		VmaAllocationCreateInfo indexAllocCreateInfo = {};
		//indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		indexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
			mAllocator, &iboInfo, &indexAllocCreateInfo, &mIndexBuffer, &mIndexAllocation, &mIndexAllocationInfo);

		memcpy(mIndexAllocationInfo.pMappedData, indices.data(), (size_t)indexMemorySize);

		std::vector<VkBuffer> vertexBuffers = { mVertexBuffer };
		std::vector<VkBuffer> indexBuffers = { mIndexBuffer };

		createCommandBuffers(
			mDevice, 
			mCommandPool, 
			mRenderPass, 
			mSwapchain.swapchainExtent, 
			mGraphicsPipeline, 
			mFramebuffers, 
			vertexBuffers, 
			indexBuffers,
			mCommandBuffers,
			mPipelineLayout,
			mDescriptorSets);

		mImageAvailable = createSemaphore(mDevice);
		mRenderFinished = createSemaphore(mDevice);

		hvk::Resource<VkImage> textureResource = createTextureImage(mDevice, mAllocator, mCommandPool, mGraphicsQueue);
	}

	void VulkanApp::init() {
		try {
			std::cout << "init vulkan" << std::endl;
			initializeVulkan();
			enableVulkanValidationLayers();
			std::cout << "init device" << std::endl;
			initializeDevice();
			std::cout << "init allocator" << std::endl;
			initializeAllocator();
			std::cout << "init renderer" << std::endl;
			initializeRenderer();
			std::cout << "done initializing" << std::endl;
		} catch (const std::runtime_error& error) {
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

		// update current UBO with model view projection
		hvk::UniformBufferObject ubo = {};
		/*
		glm::mat4 position = glm::rotate(glm::mat4(1.0f), 0.f, glm::vec3(0.f, 0.f, 1.f));
		ubo.modelViewProj = glm::perspective(
			glm::radians(45.0f), 
			mSwapchain.swapchainExtent.width / (float)mSwapchain.swapchainExtent.height, 
			0.1f, 
			10.0f) * position;
		ubo.modelViewProj[1][1] *= -1; // Flip Y value of the clip coordinates
		*/
		glm::mat4 identity = glm::mat4(1.0f);
		ubo.model = glm::translate(identity, glm::vec3(0.f, 0.f, 0.f));
		ubo.view = glm::lookAt(glm::vec3(0.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
		ubo.modelViewProj = glm::perspective(
			glm::radians(45.0f),
			mSwapchain.swapchainExtent.width / (float)mSwapchain.swapchainExtent.height,
			0.1f,
			10.0f) * ubo.view * ubo.model;
		ubo.modelViewProj[1][1] *= -1;
		//memcpy(indexAllocInfo.pMappedData, indices.data(), (size_t)indexMemorySize);
		memcpy(mUniformAllocations[imageIndex].pMappedData, &ubo, sizeof(ubo));

		//auto testData = reinterpret_cast<hvk::Vertex*>(mVertexAllocationInfo.pMappedData);
		//std::cout << testData->pos[0] << ", " << testData->pos[1] << ", " << testData->pos[2] << std::endl;

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