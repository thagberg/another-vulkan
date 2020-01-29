#pragma once

#include <array>
#include <vector>
#include <memory>
#include <variant>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "vk_mem_alloc.h"
#include "tiny_gltf.h"

//#include "ResourceManager.h"
#include "HvkUtil.h"


namespace hvk {

	typedef std::vector<VkImageView> SwapchainImageViews;
	typedef std::vector<VkImage> SwapchainImages;
	typedef std::vector<VkFramebuffer> FrameBuffers;
	typedef std::vector<VkCommandBuffer> CommandBuffers;
	typedef std::vector<VkDescriptorSet> DescriptorSets;
	//typedef std::shared_ptr<GLFWwindow, void(*)(GLFWwindow*)> window_ptr;
	typedef std::shared_ptr<GLFWwindow> window_ptr;
	typedef uint16_t VertIndex;


	template <class T>
	struct RuntimeResource {
		T memoryResource;
		VmaAllocation allocation;
	};

	template <class T>
	struct Resource {
		T memoryResource;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};
	typedef std::vector<Resource<VkBuffer>> UniformBufferResources;

	struct TextureMap
	{
		RuntimeResource<VkImage> texture;
		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
	};

	struct QueueFamilies {
		uint32_t graphics;
		uint32_t submit;
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


	struct UniformBufferObject {
		COMP3_4_ALIGN(float) glm::mat4 model;
		COMP3_4_ALIGN(float) glm::mat4 view;
		COMP3_4_ALIGN(float) glm::mat4 modelViewProj;
		COMP3_4_ALIGN(float) glm::vec3 cameraPos;
	};

	struct UniformLight {
		COMP3_4_ALIGN(float) glm::vec3 lightPos;
		COMP3_4_ALIGN(float) glm::vec3 lightColor;
		COMP1_ALIGN(float) float lightIntensity;
		COMP1_ALIGN(float) float constant;
		COMP1_ALIGN(float) float linear;
		COMP1_ALIGN(float) float quadratic;
	};

	struct AmbientLight {
		glm::vec3 lightColor;
		float lightIntensity;
	};

	struct DirectionalLight {
		COMP3_4_ALIGN(float) glm::vec3 direction;
		COMP3_4_ALIGN(float) glm::vec3 lightColor;
		COMP1_ALIGN(float) float lightIntensity;
	};

	template<size_t n>
	struct UniformLightObject {
		COMP1_ALIGN(uint32_t) uint32_t numLights;
		COMP3_4_ALIGN(float) std::array<UniformLight, n> lights;
		COMP3_4_ALIGN(float) AmbientLight ambient;
		DirectionalLight directional;
	};

	struct UiPushConstant {
		COMP2_ALIGN(float) glm::vec2 scale;
		COMP2_ALIGN(float) glm::vec2 pos;
	};

	struct PBRWeight {
		COMP1_ALIGN(float) float roughness;
		COMP1_ALIGN(float) float metallic;
	};

	struct PushConstant {
		COMP1_ALIGN(float) float gamma;
		COMP1_ALIGN(bool) bool sRGBTextures;
		COMP1_ALIGN(float) PBRWeight pbrWeight;
	};

	struct GammaSettings {
		float gamma;
	};

	struct ExposureSettings {
		float exposure;
	};

	struct RoughnessSettings {
		float roughness;
	};

	struct SkySettings {
		float gamma;
		float lod;
	};

	struct VertexInfo 
	{
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	};

}
