#pragma once

#include <vector>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "stb_image.h"

#include "types.h"

namespace hvk {
	GLFWwindow* initializeWindow(int width, int height, const char* windowTitle);

	std::vector<const char*> getRequiredExtensions();

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance, 
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
		VkDebugUtilsMessengerEXT* pDebugMesenger);


	VkResult createSwapchain(
		VkPhysicalDevice physicalDevice, 
		VkDevice device, 
		VkSurfaceKHR surface, 
		int width, 
		int height, 
		hvk::Swapchain& swapchain);

	VkSubpassDependency createSubpassDependency(
		uint32_t srcSubpass = VK_SUBPASS_EXTERNAL,
		uint32_t dstSubpass = 0,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VkAccessFlags srcAccessMask = 0,
		VkAccessFlags dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VkDependencyFlags dependencyFlags=0);


	VkSemaphore createSemaphore(VkDevice device);


	VkCommandPool createCommandPool(VkDevice device, int queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);


	void createDescriptorPool(
		VkDevice device,
		std::vector<VkDescriptorPoolSize>& poolSizes,
		uint32_t maxSets,
		VkDescriptorPool& pool);

	void createDescriptorSetLayout(
		VkDevice device,
		std::vector<VkDescriptorSetLayoutBinding>& bindings,
		VkDescriptorSetLayout& descriptorSetLayout);

	void allocateDescriptorSets(
		VkDevice device,
		VkDescriptorPool& pool,
		VkDescriptorSet& descriptorSet,
		std::vector<VkDescriptorSetLayout> layouts);

	VkDescriptorSetLayoutBinding generateUboLayoutBinding(
		uint32_t binding,
		uint32_t descriptorCount,
		VkShaderStageFlags flags=VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayoutBinding generateSamplerLayoutBinding(
		uint32_t binding,
		uint32_t descriptorCount,
		VkShaderStageFlags flags=VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineDepthStencilStateCreateInfo createDepthStencilState(
		bool depthTest = VK_TRUE,
		bool depthWrite = VK_TRUE,
		bool stencilTest = VK_FALSE,
		float minDepthBounds = 0.f,
		float maxDepthBounds = 1.f,
		VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL);

	VkWriteDescriptorSet createDescriptorBufferWrite(
		std::vector<VkDescriptorBufferInfo>& bufferInfos,
		VkDescriptorSet& descriptorSet,
		uint32_t binding);

	VkWriteDescriptorSet createDescriptorImageWrite(
		std::vector<VkDescriptorImageInfo>& imageInfos,
		VkDescriptorSet& descriptorSet,
		uint32_t binding);

	void writeDescriptorSets(
		VkDevice device,
		std::vector<VkWriteDescriptorSet>& descriptorWrites);

	template <VkDescriptorType T>
	void _buildPoolSizes(std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t count)
	{
		poolSizes.push_back({
			T,
			count});
	}

	template <VkDescriptorType T, VkDescriptorType... Args>
	void _buildPoolSizes(std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t count, uint32_t counts...)
	{
		poolSizes.push_back({
			T,
			count});
		_buildPoolSizes<Args...>(poolSizes, counts);
	}

	template <VkDescriptorType... Args>
	std::vector<VkDescriptorPoolSize> createPoolSizes(const uint32_t count, const uint32_t counts...)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve(sizeof...(Args));
		_buildPoolSizes<Args...>(poolSizes, count, counts);
		return poolSizes;
	}

	template <VkDescriptorType T>
	std::vector<VkDescriptorPoolSize> createPoolSizes(const uint32_t count)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve(1);
		_buildPoolSizes<T>(poolSizes, count);
		return poolSizes;
	}

	template <typename T>
	void fillVertexInfo(VertexInfo& vertexInfo) 
	{
		vertexInfo.bindingDescription = T::getBindingDescription();
		vertexInfo.attributeDescriptions = T::getAttributeDescriptions();
		vertexInfo.vertexInputInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			0,
			1,
			&vertexInfo.bindingDescription,
			static_cast<uint32_t>(vertexInfo.attributeDescriptions.size()),
			vertexInfo.attributeDescriptions.data()
		};
	}
}

#ifdef HVK_UTIL_IMPLEMENTATION
//#define HVK_UTIL_IMPLEMENTATION

#include <assert.h>
#include <fstream>
#include <iostream>


#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#endif

#ifndef TINYGLTF_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#endif

#define BYTES_PER_PIXEL 4

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
            }
            else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                bestMode = presentMode;
            }
        }

        return bestMode;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D extent = { width, height };

            extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
            extent.width = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

            return extent;
        }
    }

	/*RenderTarget createRenderTarget(
		VkDevice device,
		VkSurfaceFormatKHR swapSurfaceFormat,
		VkPresentModeKHR presentMode,
		VkExtent2D extent,
		uint16_t numSwaps) {

		RenderTarget renderTarget;

		return renderTarget;
	}*/

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


	VkSubpassDependency createSubpassDependency(
		uint32_t srcSubpass,
		uint32_t dstSubpass,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkDependencyFlags dependencyFlags)
	{
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = srcSubpass;
        dependency.dstSubpass = dstSubpass;
        dependency.srcStageMask = srcStageMask;
        dependency.dstStageMask = dstStageMask;
        dependency.srcAccessMask = srcAccessMask;
        dependency.dstAccessMask = dstAccessMask;
		dependency.dependencyFlags = dependencyFlags;

		return dependency;
	}


  //  VkRenderPass createRenderPass(VkDevice device, VkFormat swapchainImageFormat) {
  //      VkAttachmentDescription colorAttachment = createColorAttachment(swapchainImageFormat);
  //      VkAttachmentDescription depthAttachment = createDepthAttachment();

		//return createRenderPass(device, swapchainImageFormat, &colorAttachment, &depthAttachment);
  //  }

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

	VkPipelineDepthStencilStateCreateInfo createDepthStencilState (
		bool depthTest,
		bool depthWrite,
		bool stencilTest,
		float minDepthBounds,
		float maxDepthBounds,
		VkCompareOp depthCompareOp)
	{
		VkPipelineDepthStencilStateCreateInfo depthCreate = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depthCreate.depthTestEnable = depthTest;
		depthCreate.depthWriteEnable = depthWrite;
		depthCreate.stencilTestEnable = stencilTest;
		depthCreate.depthCompareOp = depthCompareOp;
		depthCreate.depthBoundsTestEnable = VK_FALSE;
		depthCreate.minDepthBounds = minDepthBounds;
		depthCreate.maxDepthBounds = maxDepthBounds;

		return depthCreate;
	}

	//VkPipelineRasterizationStateCreateInfo createRasterizerState(
	//	VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT,
	//	VkFrontFace frontFace=
	//)

	VkPipeline createCustomizedGraphicsPipeline(
		VkDevice device,
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		VkFrontFace frontFace,
		const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
		const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
		const VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
		const VkPipelineDepthStencilStateCreateInfo& depthStencilInfo,
		const std::vector<VkPipelineColorBlendAttachmentState>& blendAttachments) {

		VkPipeline graphicsPipeline;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = nullptr;
        viewportState.scissorCount = 1;
        viewportState.pScissors = nullptr;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = frontFace;
        //rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
		colorBlending.pAttachments = blendAttachments.data();

		std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicCreate = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicCreate.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicCreate.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencilInfo;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicCreate;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

		assert(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) == VK_SUCCESS);

        return graphicsPipeline;
	}

    VkPipeline createGraphicsPipeline(
        VkDevice device,
        VkExtent2D swapchainExtent,
        VkRenderPass renderPass,
        VkPipelineLayout pipelineLayout) {

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
        //rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

		VkPipelineDepthStencilStateCreateInfo depthCreate = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		depthCreate.depthTestEnable = VK_TRUE;
		depthCreate.depthWriteEnable = VK_TRUE;
		depthCreate.depthCompareOp = VK_COMPARE_OP_LESS;
		depthCreate.depthBoundsTestEnable = VK_FALSE;
		depthCreate.minDepthBounds = 0.0f;
		depthCreate.maxDepthBounds = 1.0f;
		depthCreate.stencilTestEnable = VK_FALSE;

		std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicCreate = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicCreate.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicCreate.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        //pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pViewportState = nullptr;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthCreate;
        pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicCreate;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

		assert(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) == VK_SUCCESS);

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);

        return graphicsPipeline;
    }

    VkCommandPool createCommandPool(
        VkDevice device, 
        int queueFamilyIndex, 
        VkCommandPoolCreateFlags flags) {

        VkCommandPool commandPool;

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = flags;

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

    VkSemaphore createSemaphore(VkDevice device) {
        VkSemaphore semaphore;

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		assert(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) == VK_SUCCESS);

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

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Descriptor Set Layout");
        }

        return descriptorSetLayout;
    }

    VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t descriptorCount, uint32_t numSets) {
        VkDescriptorPool descriptorPool;

        std::array<VkDescriptorPoolSize, 2> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = descriptorCount;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = descriptorCount;

        VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
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
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate Descriptor Sets");
        }

        return descriptorSets;
    }


	// TODO: Add support for mipmaps





	void createDescriptorPool(
		VkDevice device,
		std::vector<VkDescriptorPoolSize>& poolSizes,
		uint32_t maxSets,
		VkDescriptorPool& pool)
	{
		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;

		assert(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) == VK_SUCCESS);
	}

	VkDescriptorSetLayoutBinding generateUboLayoutBinding(
		uint32_t binding,
		uint32_t descriptorCount,
		VkShaderStageFlags flags)
	{
		return VkDescriptorSetLayoutBinding {
			binding,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			descriptorCount,
			flags,
			nullptr
		};
	}

	VkDescriptorSetLayoutBinding generateSamplerLayoutBinding(
		uint32_t binding,
		uint32_t descriptorCount,
		VkShaderStageFlags flags)
	{
		return VkDescriptorSetLayoutBinding{
			binding,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			descriptorCount,
			flags,
			nullptr
		};
	}

	void createDescriptorSetLayout(
		VkDevice device,
		std::vector<VkDescriptorSetLayoutBinding>& bindings,
		VkDescriptorSetLayout& descriptorSetLayout)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		assert(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) == VK_SUCCESS);
	}

	void allocateDescriptorSets(
		VkDevice device,
		VkDescriptorPool& pool,
		VkDescriptorSet& descriptorSet,
		std::vector<VkDescriptorSetLayout> layouts)
	{
		VkDescriptorSetAllocateInfo alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		alloc.descriptorPool = pool;
		alloc.descriptorSetCount = layouts.size();
		alloc.pSetLayouts = layouts.data();

		assert(vkAllocateDescriptorSets(device, &alloc, &descriptorSet) == VK_SUCCESS);
	}

	VkWriteDescriptorSet createDescriptorBufferWrite(
		std::vector<VkDescriptorBufferInfo>& bufferInfos,
		VkDescriptorSet& descriptorSet,
		uint32_t binding)
	{
		return VkWriteDescriptorSet{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descriptorSet,
			binding,
			0,
			static_cast<uint32_t>(bufferInfos.size()),
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			nullptr,
			bufferInfos.data(),
			nullptr
		};
	}

	VkWriteDescriptorSet createDescriptorImageWrite(
		std::vector<VkDescriptorImageInfo>& imageInfos,
		VkDescriptorSet& descriptorSet,
		uint32_t binding)
	{
		return VkWriteDescriptorSet{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			descriptorSet,
			binding,
			0,
			static_cast<uint32_t>(imageInfos.size()),
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			imageInfos.data(),
			nullptr,
			nullptr
		};
	}

	void writeDescriptorSets(
		VkDevice device,
		std::vector<VkWriteDescriptorSet>& descriptorWrites)
	{
		vkUpdateDescriptorSets(
			device,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr
		);
	}

}

#endif
