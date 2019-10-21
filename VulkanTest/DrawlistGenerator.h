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

        DrawlistGenerator(VulkanDevice device, VmaAllocator allocator, VkQueue mGraphicsQueue, VkRenderPass renderPass);
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
			VkQueue mGraphicsQueue,
			VkRenderPass renderPass);
		virtual void invalidate();
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
}
