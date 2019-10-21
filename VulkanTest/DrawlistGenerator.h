#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "types.h"
#include "Node.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "RenderObject.h"
#include "Light.h"

namespace hvk
{
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
		VertexInfo vertexInfo;
		VkPipelineLayout pipelineLayout;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
		VkFrontFace frontFace;
	};

    class DrawlistGenerator
    {
	protected:
        bool mInitialized;

        VulkanDevice mDevice;
        VmaAllocator mAllocator;
        VkQueue mGraphicsQueue;

        VkRenderPass mRenderPass;
        VkFence mRenderFence;
        VkSemaphore mRenderFinished;
        VkCommandPool mCommandPool;
        VkCommandBuffer mCommandBuffer;

        DrawlistGenerator(
			VulkanDevice device, 
			VmaAllocator allocator, 
			VkQueue mGraphicsQueue, 
			VkRenderPass renderPass,
			VkCommandPool commandPool);
        void setInitialized(bool init) { mInitialized = init; }

    public:
        virtual ~DrawlistGenerator();
		virtual void invalidate() = 0;

        bool getInitialized() const { return mInitialized; }
    };

	class StaticMeshGenerator : public DrawlistGenerator
	{
	private:

		struct StaticMeshRenderable 
		{
			std::shared_ptr<StaticMeshRenderObject> renderObject;

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

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorSetLayout mLightsDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mLightsDescriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;
		Resource<VkBuffer> mLightsUbo;						// should this be a ptr?

		std::vector<StaticMeshRenderable, Hallocator<StaticMeshRenderable>> mRenderables;
		std::vector<std::shared_ptr<Light>> mLights;

		void preparePipelineInfo();

	public:
        StaticMeshGenerator(
			VulkanDevice device, 
			VmaAllocator allocator, 
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool);
		virtual ~StaticMeshGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
		void addStaticMeshObject(std::shared_ptr<StaticMeshRenderObject> object);
		void addLight(std::shared_ptr<Light> light);
		VkCommandBuffer& drawFrame(
            const VkCommandBufferInheritanceInfo& inheritance,
			const VkFramebuffer& framebuffer,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera,
			const AmbientLight& ambientLight,
			VkFence waitFence);
	};

	class UiDrawGenerator : public DrawlistGenerator
	{
	private:
		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mDescriptorSet;
		VkImageView mFontView;
		VkSampler mFontSampler;
		Resource<VkBuffer> mVbo;
		Resource<VkBuffer> mIbo;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;
		VkExtent2D mWindowExtent;
	public:
		UiDrawGenerator(
			VulkanDevice device,
			VmaAllocator  allocator,
			VkQueue graphicsQueue,
			VkRenderPass renderPass,
			VkCommandPool commandPool,
			VkExtent2D windowExtent);
		virtual ~UiDrawGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass, VkExtent2D windowExtent);
		VkCommandBuffer& drawFrame(
            const VkCommandBufferInheritanceInfo& inheritance,
			VkFramebuffer& framebuffer,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			VkFence waitFence);
	};
}
