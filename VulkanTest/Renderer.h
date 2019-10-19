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

namespace hvk 
{

	struct Renderable 
	{
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

	struct DebugRenderable
	{
		
	};

	struct VertexInfo 
	{
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	};

	struct RenderPipelineInfo 
	{
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

		// provided externally
		VkRenderPass mRenderPass; // only needed for creating pipeline?  Make pipeline externally and provide that instead?
		VkExtent2D mExtent; // might not need to store this... Used for setting ImGui display size and renderpass render area
		VulkanDevice mDevice;
		VkQueue mGraphicsQueue;
		VmaAllocator mAllocator;

        // provided externally but might be implementation specific
		CameraRef mCamera;

		// created internally but directly dependent on mExtent
		VkViewport mViewport;
		VkRect2D mScissor;

		// created internally
		VkFence mRenderFence;
		VkCommandPool mCommandPool;
		VkCommandBuffer mCommandBuffer;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSetLayout mDescriptorSetLayout;

		// created internally and is also the output of drawFrame, which callers rely on
		VkSemaphore mRenderFinished;

		// pipelines -- create these externally and provide 1 per renderer?
		VkPipelineLayout mPipelineLayout;
		VkPipeline mPipeline;
		VkPipeline mNormalsPipeline;
		VkPipeline mUiPipeline;
		VkPipeline mDebugPipeline;
		VkPipelineLayout mUiPipelineLayout;

		RenderPipelineInfo mPipelineInfo;
		RenderPipelineInfo mNormalsPipelineInfo;
		RenderPipelineInfo mUiPipelineInfo;
		RenderPipelineInfo mDebugPipelineInfo;

		// these should somehow be captured in with the appropriate renderer only
		VkDescriptorSet mUiDescriptorSet;
		VkImageView mUiFontView;
		VkSampler mUiFontSampler;
		Resource<VkBuffer> mUiVbo;
		Resource<VkBuffer> mUiIbo;

		// specific to particular renderer implementations
		std::vector<Renderable> mRenderables;
		std::vector<LightRef> mLights;
		AmbientLight mAmbientLight;
		Resource<VkBuffer> mLightsUbo;
		VkDescriptorSetLayout mLightsDescriptorSetLayout;
		VkDescriptorSet mLightsDescriptorSet;

		/* what's needed for recording command buffer?
		*	framebuffer
		*	mRenderPass
		*	mCommandBuffer
		*	mRenderFence
		*	mViewport
		*	mScissor
		*	mAllocator?  -- seems to only be the case for UI
		*	
		*/
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

