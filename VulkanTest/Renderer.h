#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "types.h"
#include "Node.h"
#include "Camera.h"
#include "RenderObject.h"

namespace hvk {

	struct Renderable {
		RenderObjRef renderObject;

		Resource<VkBuffer> vbo;
		Resource<VkBuffer> ibo;
		size_t numVertices;
		uint16_t numIndices;

		Resource<VkImage> texture;
		VkImageView textureView;
		VkSampler textureSampler;
		Resource<VkBuffer> ubo;
		VkDescriptorSet descriptorSet;

		Resource<VkBuffer> normalVbo;
		size_t numNormalVertices;
	};

	class Renderer
	{
	private:
		static bool sDrawNormals;
	public:
		static bool getDrawNormals() { return sDrawNormals; }
		static void setDrawNormals(bool drawNormals) { sDrawNormals = drawNormals; }

	private:
		bool mInitialized;
		size_t mFirstRenderIndexAvailable;

		VkFence mRenderFence;
		VkCommandPool mCommandPool;
		VkCommandBuffer mCommandBuffer;
		VulkanDevice mDevice;
		VkQueue mGraphicsQueue;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSetLayout mDescriptorSetLayout;
		VkPipelineLayout mPipelineLayout;
		VkPipeline mPipeline;
		VkPipeline mNormalsPipeline;
		VkPipeline mUiPipeline;
		VkPipelineLayout mUiPipelineLayout;
		VkDescriptorSet mUiDescriptorSet;
		Resource<VkBuffer> mUiVbo;
		Resource<VkBuffer> mUiIbo;
		VkRenderPass mRenderPass;
		VkExtent2D mExtent;
		VkSemaphore mRenderFinished;

		CameraRef mCamera;
		std::vector<Renderable> mRenderables;

		VmaAllocator mAllocator;

		void recordCommandBuffer(VkFramebuffer& framebuffer);
		void findFirstRenderIndexAvailable();
		VkPipeline generatePipeline(
			VkPrimitiveTopology topology, 
			const char* vertShaderFile, 
			const char* fragShaderFile,
			VkExtent2D& extent,
			VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
			VkPipelineLayout& pipelineLayout);

	public:
		Renderer();
		~Renderer();

		void init(VulkanDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkRenderPass renderPass, CameraRef camera, VkFormat colorAttachmentFormat, VkExtent2D extent);
		void addRenderable(RenderObjRef renderObject);
		VkSemaphore drawFrame(VkFramebuffer& framebuffer, VkSemaphore* waitSemaphores=nullptr, uint32_t waitSemaphoreCount=0);
	};
}

