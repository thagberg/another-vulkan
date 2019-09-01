#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "types.h"
#include "Node.h"
#include "Camera.h"
#include "Light.h"
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

	struct VertexInfo {
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	};

	struct RenderPipelineInfo {
		VkPrimitiveTopology topology;
		const char* vertShaderFile;
		const char* fragShaderFile;
		VkExtent2D extent;
		VertexInfo vertexInfo;
		VkPipelineLayout pipelineLayout;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
		VkFrontFace frontFace;
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
		VkImageView mUiFontView;
		VkSampler mUiFontSampler;
		Resource<VkBuffer> mUiVbo;
		Resource<VkBuffer> mUiIbo;
		VkRenderPass mRenderPass;
		VkExtent2D mExtent;
		VkSemaphore mRenderFinished;
		VkViewport mViewport;
		VkRect2D mScissor;

		RenderPipelineInfo mPipelineInfo;
		RenderPipelineInfo mNormalsPipelineInfo;
		RenderPipelineInfo mUiPipelineInfo;

		CameraRef mCamera;
		std::vector<Renderable> mRenderables;

		std::vector<LightRef> mLights;
		AmbientLight mAmbientLight;
		Resource<VkBuffer> mLightsUbo;
		VkDescriptorSetLayout mLightsDescriptorSetLayout;
		VkDescriptorSet mLightsDescriptorSet;

		VmaAllocator mAllocator;

		void recordCommandBuffer(VkFramebuffer& framebuffer);
		void findFirstRenderIndexAvailable();
		VkPipeline generatePipeline(RenderPipelineInfo& pipelineInfo);

	public:
		Renderer();
		~Renderer();

		void init(VulkanDevice device, VmaAllocator allocator, VkQueue graphicsQueue, VkRenderPass renderPass, CameraRef camera, VkFormat colorAttachmentFormat, VkExtent2D extent);
		void addRenderable(RenderObjRef renderObject);
		void addLight(LightRef lightObject);
		VkSemaphore drawFrame(VkFramebuffer& framebuffer, VkSemaphore* waitSemaphores=nullptr, uint32_t waitSemaphoreCount=0);
		void updateRenderPass(VkRenderPass renderPass, VkExtent2D extent);
		void invalidateRenderer();
	};
}

