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
	};

	class Renderer
	{
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
		VkRenderPass mRenderPass;
		VkExtent2D mExtent;
		VkSemaphore mRenderFinished;

		CameraRef mCamera;
		std::vector<Renderable> mRenderables;

		VmaAllocator mAllocator;

		void recordCommandBuffer(VkFramebuffer& framebuffer);
		void findFirstRenderIndexAvailable();

	public:
		Renderer();
		~Renderer();

		void init(VulkanDevice device, VkQueue graphicsQueue, VkRenderPass renderPass, CameraRef camera, VkFormat colorAttachmentFormat, VkExtent2D extent);
		void addRenderable(RenderObjRef renderObject);
		VkSemaphore drawFrame(VkFramebuffer& framebuffer, VkSemaphore* waitSemaphores=nullptr, uint32_t waitSemaphoreCount=0);
	};
}

