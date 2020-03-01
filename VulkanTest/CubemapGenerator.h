#pragma once

#include "DrawlistGenerator.h"
#include "Camera.h"
#include "CubeMesh.h"
#include "types.h"
#include "RenderObject.h"
#include "stb_image.h"
#include "shapes.h"
#include "descriptor-util.h"
#include "pipeline-util.h"



namespace hvk
{
	template <typename PushT>
	class CubemapGenerator : public DrawlistGenerator
	{
	private:
		struct CubemapRenderable
		{
			HVK_shared<CubeMeshRenderObject> renderObject;

			Resource<VkBuffer> vbo;
			Resource<VkBuffer> ibo;
			size_t numVertices;
			uint32_t numIndices;

			Resource<VkBuffer> ubo;
			bool sRGB;
		};

		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSet mDescriptorSet;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

		HVK_shared<TextureMap> mCubeMap;
		CubemapRenderable mCubeRenderable;

		bool mDescriptorSetDirty;

		void updateDescriptorSet();
		
	public:
		CubemapGenerator(
			VkRenderPass renderPass,
			VkCommandPool commandPool,
            HVK_shared<TextureMap> skyboxMap,
			std::array<std::string, 2>& shaderFiles);
		virtual ~CubemapGenerator();
		virtual void invalidate() override;
		VkCommandBuffer& drawFrame(
			const VkCommandBufferInheritanceInfo& inheritance,
			const VkFramebuffer& framebuffer,
			const VkViewport& viewport,
			const VkRect2D& scissor,
			const Camera& camera,
			const PushT& pushSettings);
		void updateRenderPass(VkRenderPass renderPass);
		void setCubemap(HVK_shared<TextureMap> cubeMap);
	};


	template <typename PushT>
	CubemapGenerator<PushT>::CubemapGenerator(
		VkRenderPass renderPass, 
		VkCommandPool commandPool,
        HVK_shared<TextureMap> skyboxMap,
		std::array<std::string, 2>& shaderFiles):

		DrawlistGenerator(renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mDescriptorSet(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo(),
		mCubeMap(skyboxMap),
		mCubeRenderable(),
		mDescriptorSetDirty(false)
	{
        const auto& device = GpuManager::getDevice();
        const auto& allocator = GpuManager::getAllocator();

		//auto cube = createColoredCube(glm::vec3(0.f, 1.f, 0.f));
        auto cube = createEnvironmentCube();
		mCubeRenderable.renderObject = std::make_shared<CubeMeshRenderObject>(
			"Sky Box",
			nullptr,
			glm::mat4(1.f),
			cube);

		mCubeRenderable.sRGB = false;

		auto vertices = cube->getVertices();
		auto indices = cube->getIndices();
		mCubeRenderable.numVertices = static_cast<uint32_t>(vertices->size());
		mCubeRenderable.numIndices = static_cast<uint32_t>(indices->size());

		// Create VBO
		size_t vertexMemorySize = sizeof(CubeVertex) * mCubeRenderable.numVertices;
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = vertexMemorySize;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
            allocator,
			&bufferInfo,
			&allocCreateInfo,
			&mCubeRenderable.vbo.memoryResource,
			&mCubeRenderable.vbo.allocation,
			&mCubeRenderable.vbo.allocationInfo);

		memcpy(mCubeRenderable.vbo.allocationInfo.pMappedData, vertices->data(), vertexMemorySize);

		// Create IBO
        size_t indexMemorySize = sizeof(uint16_t) * mCubeRenderable.numIndices;
        VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        iboInfo.size = indexMemorySize;
        iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocCreateInfo = {};
        indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        indexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(
            allocator,
            &iboInfo,
            &indexAllocCreateInfo,
            &mCubeRenderable.ibo.memoryResource,
            &mCubeRenderable.ibo.allocation,
            &mCubeRenderable.ibo.allocationInfo);

        memcpy(mCubeRenderable.ibo.allocationInfo.pMappedData, indices->data(), indexMemorySize);

		// Create UBO
        uint32_t uboMemorySize = sizeof(hvk::UniformBufferObject);
        VkBufferCreateInfo uboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        uboInfo.size = uboMemorySize;
        uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo uniformAllocCreateInfo = {};
		uniformAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		uniformAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
            allocator,
			&uboInfo,
			&uniformAllocCreateInfo,
			&mCubeRenderable.ubo.memoryResource,
			&mCubeRenderable.ubo.allocation,
			&mCubeRenderable.ubo.allocationInfo);

		// Create descriptor pool
		auto poolSizes = util::descriptor::template createPoolSizes<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(1, 1);
		util::descriptor::createDescriptorPool(device, poolSizes, 1, mDescriptorPool);

		// Create descriptor set layout
		auto uboLayoutBinding = util::descriptor::generateUboLayoutBinding(0, 1);
		auto skySamplerBinding = util::descriptor::generateSamplerLayoutBinding(1, 1);

		std::vector<decltype(uboLayoutBinding)> bindings = {
			uboLayoutBinding,
			skySamplerBinding
		};
		util::descriptor::createDescriptorSetLayout(device, bindings, mDescriptorSetLayout);

		// Create descriptor set
		std::vector<VkDescriptorSetLayout> layouts = { mDescriptorSetLayout };
		util::descriptor::allocateDescriptorSets(device, mDescriptorPool, mDescriptorSet, layouts);

		// Update descriptor set
		VkDescriptorBufferInfo dsBufferInfo = {
			mCubeRenderable.ubo.memoryResource,
			0,
			sizeof(hvk::UniformBufferObject)
		};

		VkDescriptorImageInfo imageInfo = {
			mCubeMap->sampler,
			mCubeMap->view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		std::vector<VkDescriptorBufferInfo> bufferInfos = { dsBufferInfo };
		auto bufferWrite = util::descriptor::createDescriptorBufferWrite(bufferInfos, mDescriptorSet, 0);

		std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
		auto imageWrite = util::descriptor::createDescriptorImageWrite(imageInfos, mDescriptorSet, 1);

		std::vector<VkWriteDescriptorSet> descriptorWrites = {
			bufferWrite,
            imageWrite
		};
		util::descriptor::writeDescriptorSets(device, descriptorWrites);


		// Prepare pipeline
		VkPushConstantRange push = {};
		push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		push.offset = 0;
		push.size = sizeof(PushT);

		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = static_cast<uint32_t>(layouts.size());
		layoutCreate.pSetLayouts = layouts.data();
		layoutCreate.pushConstantRangeCount = 1;
		layoutCreate.pPushConstantRanges = &push;
		assert(vkCreatePipelineLayout(device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		util::pipeline::template fillVertexInfo<CubeVertex>(mPipelineInfo.vertexInfo);

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		mPipelineInfo.blendAttachments = { blendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = shaderFiles[0];
		mPipelineInfo.fragShaderFile = shaderFiles[1];
		mPipelineInfo.depthStencilState = util::pipeline::createDepthStencilState(true, true);
		mPipelineInfo.rasterizationState = util::pipeline::createRasterizationState();

		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
		

		setInitialized(true);
	}

	template <typename PushT>
	void CubemapGenerator<PushT>::updateDescriptorSet()
	{
		// Update descriptor set
		VkDescriptorBufferInfo dsBufferInfo = {
			mCubeRenderable.ubo.memoryResource,
			0,
			sizeof(hvk::UniformBufferObject)
		};

		VkDescriptorImageInfo imageInfo = {
			mCubeMap->sampler,
			mCubeMap->view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		std::vector<VkDescriptorBufferInfo> bufferInfos = { dsBufferInfo };
		auto bufferWrite = util::descriptor::createDescriptorBufferWrite(bufferInfos, mDescriptorSet, 0);

		std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
		auto imageWrite = util::descriptor::createDescriptorImageWrite(imageInfos, mDescriptorSet, 1);

		std::vector<VkWriteDescriptorSet> descriptorWrites = {
			bufferWrite,
            imageWrite
		};
		util::descriptor::writeDescriptorSets(GpuManager::getDevice(), descriptorWrites);
	}

	template <typename PushT>
	void CubemapGenerator<PushT>::setCubemap(HVK_shared<TextureMap> cubeMap)
	{
		mCubeMap.reset();
		mCubeMap = cubeMap;
		//updateDescriptorSet();
		mDescriptorSetDirty = true;
	}

	template <typename PushT>
	void CubemapGenerator<PushT>::updateRenderPass(VkRenderPass renderPass)
	{
		mColorRenderPass = renderPass;
		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
		setInitialized(true);
	}

	template <typename PushT>
	CubemapGenerator<PushT>::~CubemapGenerator()
	{
        const auto& device = GpuManager::getDevice();
        const auto& allocator = GpuManager::getAllocator();

		vmaDestroyBuffer(allocator, mCubeRenderable.vbo.memoryResource, mCubeRenderable.vbo.allocation);
		vmaDestroyBuffer(allocator, mCubeRenderable.ibo.memoryResource, mCubeRenderable.ibo.allocation);
		vmaDestroyBuffer(allocator, mCubeRenderable.ubo.memoryResource, mCubeRenderable.ubo.allocation);
		vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);
		vkDestroyPipeline(device, mPipeline, nullptr);
		vkDestroyPipelineLayout(device, mPipelineInfo.pipelineLayout, nullptr);
		mCubeMap.reset();
	}

	template <typename PushT>
	void CubemapGenerator<PushT>::invalidate()
	{
		setInitialized(false);
		vkDestroyPipeline(GpuManager::getDevice(), mPipeline, nullptr);
	}

	template <typename PushT>
	VkCommandBuffer& CubemapGenerator<PushT>::drawFrame(
		const VkCommandBufferInheritanceInfo& inheritance,
		const VkFramebuffer& framebuffer,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const Camera& camera,
		const PushT& pushSettings)
	{
		// If cubemap was updated we need to update the descriptor set
		if (mDescriptorSetDirty)
		{
			mDescriptorSetDirty = false;
			updateDescriptorSet();
		}

		// Update UBO
		UniformBufferObject ubo = {};
		ubo.model = camera.getWorldTransform();
		//ubo.model[1][1] *= -1;
		ubo.view = camera.getViewTransform();
		//ubo.modelViewProj = camera.getProjection() * ubo.view * ubo.model;
		ubo.modelViewProj = camera.getProjection() * glm::mat4(glm::mat3(ubo.view));
		ubo.cameraPos = camera.getWorldPosition();
		memcpy(mCubeRenderable.ubo.allocationInfo.pMappedData, &ubo, sizeof(ubo));

		// begin command buffer
		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBegin.pInheritanceInfo = &inheritance;

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		// bind viewport and scissor
		vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mCubeRenderable.vbo.memoryResource, offsets);
		vkCmdBindIndexBuffer(mCommandBuffer, mCubeRenderable.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(
			mCommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			mPipelineInfo.pipelineLayout,
			0,
			1,
			&mDescriptorSet,
			0,
			nullptr);

		//PushConstant push = { gammaSettings.gamma, mCubeRenderable.sRGB };
		//vkCmdPushConstants(
		//	mCommandBuffer, 
		//	mPipelineInfo.pipelineLayout, 
		//	VK_SHADER_STAGE_FRAGMENT_BIT, 
		//	0, 
		//	sizeof(PushConstant), 
		//	&push);

		vkCmdPushConstants(
			mCommandBuffer, 
			mPipelineInfo.pipelineLayout, 
			VK_SHADER_STAGE_FRAGMENT_BIT, 
			0, 
			sizeof(PushT), 
			&pushSettings);

		vkCmdDrawIndexed(mCommandBuffer, mCubeRenderable.numIndices, 1, 0, 0, 0);
		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}
}
