#include <memory>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#define VertexPositionFormat VK_FORMAT_R32G32B32_SFLOAT
#define VertexColorFormat VK_FORMAT_R32G32B32_SFLOAT

namespace hvk {

	typedef std::vector<VkImageView> SwapchainImageViews;
	typedef std::vector<VkImage> SwapchainImages;
	typedef std::vector<VkFramebuffer> FrameBuffers;
	typedef std::vector<VkCommandBuffer> CommandBuffers;
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

			auto positionDesc = attributeDescriptions[0];
			positionDesc.binding = 0;
			positionDesc.location = 0;
			positionDesc.format = VertexPositionFormat;
			positionDesc.offset = offsetof(Vertex, pos);

			auto colorDesc = attributeDescriptions[1];
			colorDesc.binding = 0;
			colorDesc.location = 1;
			colorDesc.format = VertexColorFormat;
			colorDesc.offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
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
		VkDeviceMemory mVertexBufferMemory;
		int mWindowWidth, mWindowHeight;

		void initializeVulkan();
		void enableVulkanValidationLayers();
		void initializeDevice();
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