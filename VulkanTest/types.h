#pragma once

#include <array>
#include <vector>
#include <memory>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "vk_mem_alloc.h"

#define VertexPositionFormat VK_FORMAT_R32G32B32_SFLOAT
#define VertexColorFormat VK_FORMAT_R32G32B32_SFLOAT
#define VertexUVFormat VK_FORMAT_R32G32_SFLOAT

namespace hvk {

	typedef std::vector<VkImageView> SwapchainImageViews;
	typedef std::vector<VkImage> SwapchainImages;
	typedef std::vector<VkFramebuffer> FrameBuffers;
	typedef std::vector<VkCommandBuffer> CommandBuffers;
	typedef std::vector<VkDescriptorSet> DescriptorSets;
	//typedef std::shared_ptr<GLFWwindow, void(*)(GLFWwindow*)> window_ptr;
	typedef std::shared_ptr<GLFWwindow> window_ptr;

	template <class T>
	struct Resource {
		T memoryResource;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};
	typedef std::vector<Resource<VkBuffer>> UniformBufferResources;

	struct QueueFamilies {
		int graphics;
		int submit;
	};

	struct VulkanDevice {
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		QueueFamilies queueFamilies;
	};

	struct Swapchain {
		VkSwapchainKHR swapchain;
		VkFormat swapchainImageFormat;
		VkExtent2D swapchainExtent;
	};

	struct RenderTarget {
		uint16_t num;
		Swapchain swapchain;
		FrameBuffers frameBuffers;
		SwapchainImages swapImages;
		SwapchainImageViews swapImageViews;
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VertexPositionFormat;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VertexColorFormat;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VertexUVFormat;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 modelViewProj;
	};
}
