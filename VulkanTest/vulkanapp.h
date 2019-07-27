#include <memory>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "vk_mem_alloc.h"

#define VertexPositionFormat VK_FORMAT_R32G32B32_SFLOAT
#define VertexColorFormat VK_FORMAT_R32G32B32_SFLOAT

namespace hvk {

	typedef std::vector<VkImageView> SwapchainImageViews;
	typedef std::vector<VkImage> SwapchainImages;
	typedef std::vector<VkFramebuffer> FrameBuffers;
	typedef std::vector<VkCommandBuffer> CommandBuffers;
	typedef std::vector<VkDescriptorSet> DescriptorSets;
	typedef std::vector<VkBuffer> UniformBuffers;
	typedef std::vector<VmaAllocationInfo> UniformAllocationInfos;
	//typedef std::shared_ptr<GLFWwindow, void(*)(GLFWwindow*)> window_ptr;
	typedef std::shared_ptr<GLFWwindow> window_ptr;

	struct Swapchain {
		VkSwapchainKHR swapchain;
		VkFormat swapchainImageFormat;
		VkExtent2D swapchainExtent;
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VertexPositionFormat;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VertexColorFormat;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

	template <class T>
	struct Resource {
		T memoryResource;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 modelViewProj;
	};

	class VulkanApp {
	private:
		VkInstance mInstance;
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;
		VkQueue mGraphicsQueue;
		VkPipelineLayout mPipelineLayout;
		VkPipeline mGraphicsPipeline;
		VkSurfaceKHR mSurface;
		VkRenderPass mRenderPass;
		VkCommandPool mCommandPool;
		CommandBuffers mCommandBuffers;
		SwapchainImageViews mImageViews;
		SwapchainImages mSwapchainImages;
		FrameBuffers mFramebuffers;
		hvk::Swapchain mSwapchain;
		VkSemaphore mImageAvailable;
		VkSemaphore mRenderFinished;
		uint32_t mGraphicsIndex;
		window_ptr mWindow;
		VkBuffer mVertexBuffer;
		VkBuffer mIndexBuffer;
		// TODO: create a struct that holds a buffer/image, allocation, and allocation info
		VmaAllocation mVertexAllocation;
		VmaAllocationInfo mVertexAllocationInfo;
		VmaAllocation mIndexAllocation;
		VmaAllocationInfo mIndexAllocationInfo;
		VkImage mTextureImage;
		VmaAllocation mTextureAllocation;
		VmaAllocationInfo mTextureAllocationInfo;
		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		DescriptorSets mDescriptorSets;
		UniformBuffers mUniformBuffers;
		UniformAllocationInfos mUniformAllocations;
		VmaAllocator mAllocator;
		int mWindowWidth, mWindowHeight;

		void initializeVulkan();
		void enableVulkanValidationLayers();
		void initializeDevice();
		void initializeAllocator();
		void initializeRenderer();
		void drawFrame();

	public:
		VulkanApp(int width, int height, const char* windowTitle);
		~VulkanApp();

		void init();
		void run();

		window_ptr getWindow() { 
			return mWindow; 
		}
	};
}