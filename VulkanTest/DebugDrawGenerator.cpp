#include "pch.h"
#include "DebugDrawGenerator.h"

#include "descriptor-util.h"
#include "pipeline-util.h"
#include "GpuManager.h"

namespace hvk
{

	DebugDrawGenerator::DebugDrawGenerator(
			VkRenderPass renderPass,
			VkCommandPool commandPool) :

		DrawlistGenerator(renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo()
	{
        const VkDevice& device = GpuManager::getDevice();

		/***************
		 Create descriptor set layout and descriptor pool
		***************/
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		assert(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mDescriptorSetLayout) == VK_SUCCESS);

		auto poolSizes = util::descriptor::createPoolSizes<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(MAX_UBOS);
		util::descriptor::createDescriptorPool(device, poolSizes, MAX_DESCRIPTORS, mDescriptorPool);

		/**************
		Set up pipeline
		**************/
		std::array<VkDescriptorSetLayout, 1> dsLayouts = { mDescriptorSetLayout };
		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = static_cast<uint32_t>(dsLayouts.size());
		layoutCreate.pSetLayouts = dsLayouts.data();
		layoutCreate.pushConstantRangeCount = 0;

		assert(vkCreatePipelineLayout(device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		util::pipeline::fillVertexInfo<ColorVertex>(mPipelineInfo.vertexInfo);

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		mPipelineInfo.blendAttachments = { blendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = "shaders/compiled/normal_v.spv";
		mPipelineInfo.fragShaderFile = "shaders/compiled/normal_f.spv";
		mPipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		mPipelineInfo.depthStencilState = util::pipeline::createDepthStencilState();

		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);

		setInitialized(true);
	}


	DebugDrawGenerator::~DebugDrawGenerator()
	{
        const auto& device = GpuManager::getDevice();
        const auto& allocator = GpuManager::getAllocator();

        vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);

        vkDestroyPipeline(device, mPipeline, nullptr);
        vkDestroyPipelineLayout(device, mPipelineInfo.pipelineLayout, nullptr);
	}

	void DebugDrawGenerator::invalidate()
	{
		setInitialized(false);
		vkDestroyPipeline(GpuManager::getDevice(), mPipeline, nullptr);
	}

	void DebugDrawGenerator::updateRenderPass(VkRenderPass renderPass)
	{
		mColorRenderPass = renderPass;
		mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
		setInitialized(true);
	}

	void DebugDrawGenerator::addDebugMeshObject(std::shared_ptr<DebugMeshRenderObject> object)
	{
        const auto& device = GpuManager::getDevice();
        const auto& allocator = GpuManager::getAllocator();

		DebugMeshRenderable newRenderable;
		newRenderable.renderObject = object;

		const DebugMesh::ColorVertices vertices = object->getVertices();
		const DebugMesh::Indices indices = object->getIndices();
		newRenderable.numVertices = static_cast<uint32_t>(vertices->size());
		newRenderable.numIndices = static_cast<uint32_t>(indices->size());

		// Create vertex buffer
        size_t vertexMemorySize = sizeof(ColorVertex) * newRenderable.numVertices;
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
            &newRenderable.vbo.memoryResource,
            &newRenderable.vbo.allocation,
            &newRenderable.vbo.allocationInfo);

        memcpy(newRenderable.vbo.allocationInfo.pMappedData, vertices->data(), vertexMemorySize);

		// Create index buffer
        uint32_t indexMemorySize = sizeof(uint16_t) * newRenderable.numIndices;
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
            &newRenderable.ibo.memoryResource,
            &newRenderable.ibo.allocation,
            &newRenderable.ibo.allocationInfo);

        memcpy(newRenderable.ibo.allocationInfo.pMappedData, indices->data(), (size_t)indexMemorySize);

        // create UBOs
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
			&newRenderable.ubo.memoryResource,
			&newRenderable.ubo.allocation,
			&newRenderable.ubo.allocationInfo);

		// TODO: pre-allocate a number of descriptor sets for renderables
		// create descriptor set
		VkDescriptorSetAllocateInfo dsAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		dsAlloc.descriptorPool = mDescriptorPool;
		dsAlloc.descriptorSetCount = 1;
		dsAlloc.pSetLayouts = &mDescriptorSetLayout;

		assert(vkAllocateDescriptorSets(device, &dsAlloc, &newRenderable.descriptorSet) == VK_SUCCESS);

		// Update descriptor set
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = newRenderable.ubo.memoryResource;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(hvk::UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrite.dstSet = newRenderable.descriptorSet;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			std::array<VkWriteDescriptorSet, 1> descriptorWrites = { descriptorWrite };

			vkUpdateDescriptorSets(
                device, 
                static_cast<uint32_t>(descriptorWrites.size()), 
                descriptorWrites.data(), 
                0, 
                nullptr);
		}

		mRenderables.push_back(newRenderable);
	}

	VkCommandBuffer& DebugDrawGenerator::drawFrame(
		const VkCommandBufferInheritanceInfo& inheritance, 
		const VkFramebuffer& framebuffer, 
		const VkViewport& viewport, 
		const VkRect2D& scissor, 
		const Camera& camera)
	{
		/****************
		 update MVP uniform for renderables
		****************/
		for (const auto& renderable : mRenderables) {
			UniformBufferObject ubo = {};
			ubo.model = renderable.renderObject->getWorldTransform();
			ubo.model[1][1] *= -1; // flipping Y coord for Vulkan
			ubo.view = camera.getViewTransform();
			ubo.modelViewProj = camera.getProjection() * ubo.view * ubo.model;
			ubo.cameraPos = camera.getWorldPosition();
			memcpy(renderable.ubo.allocationInfo.pMappedData, &ubo, sizeof(ubo));
		}

		/******************
		 record commands
		******************/

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBegin.pInheritanceInfo = &inheritance;

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		// bind viewport and scissor
		vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		// draw renderables
		for (const auto& renderable : mRenderables) {
			vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &renderable.vbo.memoryResource, offsets);
			vkCmdBindIndexBuffer(mCommandBuffer, renderable.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(
				mCommandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mPipelineInfo.pipelineLayout, 
				0, 
				1,
				&renderable.descriptorSet,
				0, 
				nullptr);

			vkCmdDrawIndexed(mCommandBuffer, renderable.numIndices, 1, 0, 0, 0);
		}

		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}

	DebugDrawBinding DebugDrawGenerator::createDebugDrawBinding()
	{
		DebugDrawBinding newBinding;

		const auto& device = GpuManager::getDevice();
		const auto& allocator = GpuManager::getAllocator();

		// create UBO
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
			&newBinding.ubo.memoryResource,
			&newBinding.ubo.allocation,
			nullptr);

		// create descriptor set
		VkDescriptorSetAllocateInfo dsAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		dsAlloc.descriptorPool = mDescriptorPool;
		dsAlloc.descriptorSetCount = 1;
		dsAlloc.pSetLayouts = &mDescriptorSetLayout;

		assert(vkAllocateDescriptorSets(device, &dsAlloc, &newBinding.descriptorSet) == VK_SUCCESS);

		// update descriptor set
		std::vector<VkWriteDescriptorSet> descriptorWrites;
		std::vector<VkDescriptorBufferInfo> bufferInfos = {
			VkDescriptorBufferInfo {
				newBinding.ubo.memoryResource,
				0,
				sizeof(hvk::UniformBufferObject) } };
		auto bufferDescriptorWrite = util::descriptor::createDescriptorBufferWrite(bufferInfos, newBinding.descriptorSet, 0);
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		return newBinding;
	}
}
