#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "types.h"
#include "Node.h"
#include "Camera.h"
#include "Renderer.h"

namespace hvk
{

    class DrawlistGenerator
    {
    private:
        bool mInitialized;

        VkRenderPass mRenderPass;
        VulkanDevice mDevice;
        VkQueue mGraphicsQueue;
        VmaAllocator mAllocator;

        VkViewport mViewport;
        VkRect2D mScissor;

        VkFence mRenderFence;
        VkSemaphore mRenderFinished;
        VkCommandPool mCommandPool;
        VkCommandBuffer mCommandBuffer;
        VkDescriptorPool mDescriptorPool;
        VkDescriptorSetLayout mDescriptorSetLayout;

        DrawlistGenerator();
        void setInitialized(bool init) { mInitialized = init; }

    public:
        virtual ~DrawlistGenerator();

        bool getInitialized() const { return mInitialized; }
        virtual VkSemaphore drawFrame(const VkFramebuffer& framebuffer, const VkSemaphore* waitSemaphores = nullptr, uint32_t waitSemaphoreCount = 0) = 0;
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
			VkRenderPass renderPass);
		virtual ~StaticMeshGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
		void addStaticMeshObject(std::shared_ptr<StaticMeshRenderObject> object);
		void addLight(std::shared_ptr<Light> light);
        VkSemaphore drawFrame(
			const VkFramebuffer& framebuffer, 
			const VkViewport& viewport,
			const VkRect2D& scissor, 
			const Camera& camera,
			const AmbientLight& ambientLight,
			const VkSemaphore* waitSemaphores = nullptr, 
			uint32_t waitSemaphoreCount = 0);
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
			VkExtent2D windowExtent);
		virtual ~UiDrawGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass, VkExtent2D windowExtent);
		VkSemaphore drawFrame(
			VkFramebuffer& framebuffer, 
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const VkSemaphore* waitSemaphores = nullptr,
			uint32_t waitSemaphoreCount = 0);
	};
}
