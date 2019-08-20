#include "stdafx.h"
#include <assert.h>
#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "imgui/imgui.h"

#include "Renderer.h"
#define HVK_UTIL_IMPLEMENTATION
#include "vulkan-util.h"

#define MAX_DESCRIPTORS 20
#define NUM_INITIAL_RENDEROBJECTS 10

namespace hvk {

	bool Renderer::sDrawNormals = true;

	Renderer::Renderer() :
		mFirstRenderIndexAvailable(-1),
		mUiVbo{},
		mUiIbo{}
	{
	}


	Renderer::~Renderer()
	{
	}

	void Renderer::init(
		VulkanDevice device, 
		VmaAllocator allocator,
		VkQueue graphicsQueue,
		VkRenderPass renderPass,
		CameraRef camera, 
		VkFormat colorAttachmentFormat,
		VkExtent2D extent) {

		ImGui::CreateContext();

		mDevice = device;
		mGraphicsQueue = graphicsQueue;
		mRenderPass = renderPass;
		mExtent = extent;
		mCamera = camera;
		mAllocator = allocator;

		mRenderables.reserve(NUM_INITIAL_RENDEROBJECTS);

		// Create fence for rendering complete
		VkFenceCreateInfo fenceCreate = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreate.pNext = nullptr;
		fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		assert(vkCreateFence(mDevice.device, &fenceCreate, nullptr, &mRenderFence) == VK_SUCCESS);

		// create semaphore for rendering finished
		mRenderFinished = createSemaphore(mDevice.device);

		// Create command pool
		VkCommandPoolCreateInfo poolCreate = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreate.queueFamilyIndex = device.queueFamilies.graphics;
		poolCreate.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		assert(vkCreateCommandPool(mDevice.device, &poolCreate, nullptr, &mCommandPool) == VK_SUCCESS);

		// Allocate command buffer
		VkCommandBufferAllocateInfo bufferAlloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		bufferAlloc.commandBufferCount = 1;
		bufferAlloc.commandPool = mCommandPool;
		bufferAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		assert(vkAllocateCommandBuffers(mDevice.device, &bufferAlloc, &mCommandBuffer) == VK_SUCCESS);

		// Create descriptor set layout and descriptor pool
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerLayoutBiding = {};
		samplerLayoutBiding.binding = 1;
		samplerLayoutBiding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBiding.descriptorCount = 1;
		samplerLayoutBiding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBiding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBiding };
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		assert(vkCreateDescriptorSetLayout(mDevice.device, &layoutInfo, nullptr, &mDescriptorSetLayout) == VK_SUCCESS);

		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = MAX_DESCRIPTORS;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = MAX_DESCRIPTORS;

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = MAX_DESCRIPTORS;

		assert(vkCreateDescriptorPool(mDevice.device, &poolInfo, nullptr, &mDescriptorPool) == VK_SUCCESS);

		// Create graphics pipeline
		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = 1;
		layoutCreate.pSetLayouts = &mDescriptorSetLayout;
		layoutCreate.pushConstantRangeCount = 0;
		layoutCreate.pPushConstantRanges = nullptr;

		assert(vkCreatePipelineLayout(mDevice.device, &layoutCreate, nullptr, &mPipelineLayout) == VK_SUCCESS);

		VkVertexInputBindingDescription modelBindingDescription = hvk::Vertex::getBindingDescription();
		auto modelAttributeDescriptions = hvk::Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo modelVertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		modelVertexInputInfo.vertexBindingDescriptionCount = 1;
		modelVertexInputInfo.pVertexBindingDescriptions = &modelBindingDescription;
		modelVertexInputInfo.vertexAttributeDescriptionCount = modelAttributeDescriptions.size();
		modelVertexInputInfo.pVertexAttributeDescriptions = modelAttributeDescriptions.data();

		mPipeline = generatePipeline(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			"shaders/compiled/vert.spv",
			"shaders/compiled/frag.spv",
			extent,
			modelVertexInputInfo,
			mPipelineLayout);

		VkVertexInputBindingDescription normalBindingDescription = hvk::ColorVertex::getBindingDescription();
		auto normalAttributeDescriptions = hvk::ColorVertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo normalVertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		normalVertexInputInfo.vertexBindingDescriptionCount = 1;
		normalVertexInputInfo.pVertexBindingDescriptions = &normalBindingDescription;
		normalVertexInputInfo.vertexAttributeDescriptionCount = normalAttributeDescriptions.size();
		normalVertexInputInfo.pVertexAttributeDescriptions = normalAttributeDescriptions.data();

		mNormalsPipeline = generatePipeline(
			VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
			"shaders/compiled/normal_v.spv",
			"shaders/compiled/normal_f.spv",
			extent,
			normalVertexInputInfo,
			mPipelineLayout);

		// Initialize Imgui stuff
		unsigned char* fontTextureData;
		int fontTextWidth, fontTextHeight;
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->GetTexDataAsRGBA32(&fontTextureData, &fontTextWidth, &fontTextHeight);
		io.DisplaySize = ImVec2(mExtent.width, mExtent.height);
		io.DisplayFramebufferScale = ImVec2(1.f, 1.f);


        Resource<VkImage> uiFont = createTextureImage(
			mDevice.device, 
			mAllocator, 
			mCommandPool, 
			mGraphicsQueue,
			fontTextureData,
			fontTextWidth,
			fontTextHeight,
			4,
			8);

		VkDescriptorSetLayout uiDescriptorSetLayout;
		VkImageView uiFontView = createImageView(mDevice.device, uiFont.memoryResource, VK_FORMAT_R8G8B8A8_UNORM);
		VkSampler uiFontSampler = createTextureSampler(mDevice.device);

		VkDescriptorSetLayoutBinding uiLayoutImageBinding = {};
		uiLayoutImageBinding.binding = 0;
		uiLayoutImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		uiLayoutImageBinding.descriptorCount = 1;
		uiLayoutImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		uiLayoutImageBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo uiLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		uiLayoutInfo.bindingCount = 1;
		uiLayoutInfo.pBindings = &uiLayoutImageBinding;

		assert(vkCreateDescriptorSetLayout(mDevice.device, &uiLayoutInfo, nullptr, &uiDescriptorSetLayout) == VK_SUCCESS);

		VkDescriptorSetAllocateInfo uiAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		uiAlloc.descriptorPool = mDescriptorPool;
		uiAlloc.descriptorSetCount = 1;
		uiAlloc.pSetLayouts = &uiDescriptorSetLayout;

		assert(vkAllocateDescriptorSets(mDevice.device, &uiAlloc, &mUiDescriptorSet) == VK_SUCCESS);

		VkDescriptorImageInfo fontImageInfo = {};
		fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		fontImageInfo.imageView = uiFontView;
		fontImageInfo.sampler = uiFontSampler;

		VkWriteDescriptorSet fontDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		fontDescriptorWrite.dstSet = mUiDescriptorSet;
		fontDescriptorWrite.dstBinding = 0;
		fontDescriptorWrite.dstArrayElement = 0;
		fontDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fontDescriptorWrite.descriptorCount = 1;
		fontDescriptorWrite.pImageInfo = &fontImageInfo;

		vkUpdateDescriptorSets(mDevice.device, 1, &fontDescriptorWrite, 0, nullptr);

		VkPushConstantRange uiPushRange = {};
		uiPushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uiPushRange.size = sizeof(hvk::UiPushConstant);
		uiPushRange.offset = 0;

		VkPipelineLayoutCreateInfo uiLayoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		uiLayoutCreate.setLayoutCount = 1;
		uiLayoutCreate.pSetLayouts = &uiDescriptorSetLayout;
		uiLayoutCreate.pushConstantRangeCount = 1;
		uiLayoutCreate.pPushConstantRanges = &uiPushRange;

		assert(vkCreatePipelineLayout(mDevice.device, &uiLayoutCreate, nullptr, &mUiPipelineLayout) == VK_SUCCESS);

		VkVertexInputBindingDescription uiBindingDescription = {};
		uiBindingDescription.binding = 0;
		uiBindingDescription.stride = sizeof(ImDrawVert);
		uiBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	

		std::array<VkVertexInputAttributeDescription, 3> uiAttributeDescriptions = {};
		uiAttributeDescriptions[0].binding = 0;
		uiAttributeDescriptions[0].location = 0;
		uiAttributeDescriptions[0].format = UiVertexPositionFormat;
		uiAttributeDescriptions[0].offset = offsetof(ImDrawVert, pos);

		uiAttributeDescriptions[1].binding = 0;
		uiAttributeDescriptions[1].location = 1;
		uiAttributeDescriptions[1].format = UiVertexUVFormat;
		uiAttributeDescriptions[1].offset = offsetof(ImDrawVert, uv);

		uiAttributeDescriptions[2].binding = 0;
		uiAttributeDescriptions[2].location = 2;
		uiAttributeDescriptions[2].format = UiVertexColorFormat;
		uiAttributeDescriptions[2].offset = offsetof(ImDrawVert, col);

		VkPipelineVertexInputStateCreateInfo uiVertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		uiVertexInputInfo.vertexBindingDescriptionCount = 1;
		uiVertexInputInfo.pVertexBindingDescriptions = &uiBindingDescription;
		uiVertexInputInfo.vertexAttributeDescriptionCount = uiAttributeDescriptions.size();
		uiVertexInputInfo.pVertexAttributeDescriptions = uiAttributeDescriptions.data();

		mUiPipeline = generatePipeline(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			"shaders/compiled/ui_v.spv",
			"shaders/compiled/ui_f.spv",
			extent,
			uiVertexInputInfo,
			mUiPipelineLayout);

		mInitialized = true;
	}

	VkPipeline Renderer::generatePipeline(
		VkPrimitiveTopology topology, 
		const char* vertShaderFile, 
		const char* fragShaderFile,
		VkExtent2D& extent,
		VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
		VkPipelineLayout& pipelineLayout) {

		VkPipeline pipeline;

		auto modelVertShaderCode = readFile(vertShaderFile);
		auto modelFragShaderCode = readFile(fragShaderFile);
		VkShaderModule modelVertShaderModule = createShaderModule(mDevice.device, modelVertShaderCode);
		VkShaderModule modelFragShaderModule = createShaderModule(mDevice.device, modelFragShaderCode);

		VkPipelineShaderStageCreateInfo modelVertStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		modelVertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		modelVertStageInfo.module = modelVertShaderModule;
		modelVertStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo modelFragStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		modelFragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		modelFragStageInfo.module = modelFragShaderModule;
		modelFragStageInfo.pName = "main";

		std::vector<VkPipelineShaderStageCreateInfo> modelShaderStages = {
			modelVertStageInfo, modelFragStageInfo
		};

		VkPipelineInputAssemblyStateCreateInfo modelInputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		modelInputAssembly.topology = topology;
		modelInputAssembly.primitiveRestartEnable = VK_FALSE;

		pipeline = createCustomizedGraphicsPipeline(
			mDevice.device, 
			extent, 
			mRenderPass, 
			pipelineLayout,
			modelShaderStages,
			vertexInputInfo,
			modelInputAssembly);

		vkDestroyShaderModule(mDevice.device, modelVertShaderModule, nullptr);
		vkDestroyShaderModule(mDevice.device, modelFragShaderModule, nullptr);

		return pipeline;
	}

	void Renderer::addRenderable(RenderObjRef renderObject) {
		Renderable newRenderable;
		newRenderable.renderObject = renderObject;

		VerticesRef vertices = renderObject->getVertices();
		IndicesRef indices = renderObject->getIndices();
		newRenderable.numVertices = vertices->size();
		newRenderable.numIndices = indices->size();

		// Create vertex buffer
        uint32_t vertexMemorySize = sizeof(Vertex) * vertices->size();
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = vertexMemorySize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(
            mAllocator,
            &bufferInfo,
            &allocCreateInfo,
            &newRenderable.vbo.memoryResource,
            &newRenderable.vbo.allocation,
            &newRenderable.vbo.allocationInfo);

        memcpy(newRenderable.vbo.allocationInfo.pMappedData, vertices->data(), (size_t)vertexMemorySize);

		// Create index buffer
        uint32_t indexMemorySize = sizeof(uint16_t) * indices->size();
        VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        iboInfo.size = indexMemorySize;
        iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocCreateInfo = {};
        indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        indexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(
            mAllocator,
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
			mAllocator,
			&uboInfo,
			&uniformAllocCreateInfo,
			&newRenderable.ubo.memoryResource,
			&newRenderable.ubo.allocation,
			&newRenderable.ubo.allocationInfo);

		// create texture
		TextureRef tex = renderObject->getTexture();
        newRenderable.texture = createTextureImage(
			mDevice.device, 
			mAllocator, 
			mCommandPool, 
			mGraphicsQueue,
			tex->image.data(),
			tex->width,
			tex->height,
			tex->component,
			tex->bits);
        newRenderable.textureView = createImageView(mDevice.device, newRenderable.texture.memoryResource, VK_FORMAT_R8G8B8A8_UNORM);
        newRenderable.textureSampler = createTextureSampler(mDevice.device);

		// create descriptor set
		VkDescriptorSetAllocateInfo dsAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		dsAlloc.descriptorPool = mDescriptorPool;
		dsAlloc.descriptorSetCount = 1;
		dsAlloc.pSetLayouts = &mDescriptorSetLayout;

		assert(vkAllocateDescriptorSets(mDevice.device, &dsAlloc, &newRenderable.descriptorSet) == VK_SUCCESS);

		// Update descriptor set
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = newRenderable.ubo.memoryResource;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(hvk::UniformBufferObject);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = newRenderable.textureView;
			imageInfo.sampler = newRenderable.textureSampler;

			VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrite.dstSet = newRenderable.descriptorSet;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			VkWriteDescriptorSet imageDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			imageDescriptorWrite.dstSet = newRenderable.descriptorSet;
			imageDescriptorWrite.dstBinding = 1;
			imageDescriptorWrite.dstArrayElement = 0;
			imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageDescriptorWrite.descriptorCount = 1;
			imageDescriptorWrite.pImageInfo = &imageInfo;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = { descriptorWrite, imageDescriptorWrite };

			vkUpdateDescriptorSets(mDevice.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}

		// create buffers for rendering normals
		newRenderable.numNormalVertices = vertices->size() * 2;
        uint32_t normalMemorySize = sizeof(ColorVertex) * newRenderable.numNormalVertices;
        VkBufferCreateInfo normalBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        normalBufferInfo.size = normalMemorySize;
        normalBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo normalCreateInfo = {};
        normalCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        normalCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(
            mAllocator,
            &normalBufferInfo,
            &normalCreateInfo,
            &newRenderable.normalVbo.memoryResource,
            &newRenderable.normalVbo.allocation,
            &newRenderable.normalVbo.allocationInfo);

		int memOffset = 0;
		char* copyAddr = reinterpret_cast<char*>(newRenderable.normalVbo.allocationInfo.pMappedData);
		for (size_t i = 0; i < vertices->size(); ++i) {
			Vertex v = vertices->at(i);
			ColorVertex cv = {
				v.pos,
				glm::vec3(1.0f, 0.f, 0.f)
			};
			memcpy(copyAddr + memOffset, &cv, sizeof(cv));
			memOffset += sizeof(cv);
			glm::vec3 p = v.pos + v.normal;
			ColorVertex cv2 = {
				v.pos + v.normal * 3.f,
				glm::vec3(0.f, 1.f, 0.f)
			};
			memcpy(copyAddr + memOffset, &cv2, sizeof(cv2));
			memOffset += sizeof(cv2);
		}

		//findFirstRenderIndexAvailable();
		//mRenderables[mFirstRenderIndexAvailable] = newRenderable;
		mRenderables.push_back(newRenderable);
	}

	void Renderer::findFirstRenderIndexAvailable() {
		// TODO: actually look for the next available index
		//		 and resize the vector if necessary
		mFirstRenderIndexAvailable++;
	}

	void Renderer::recordCommandBuffer(VkFramebuffer& framebuffer) {
		assert(vkWaitForFences(mDevice.device, 1, &mRenderFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);
		assert(vkResetFences(mDevice.device, 1, &mRenderFence) == VK_SUCCESS);

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		commandBegin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBegin.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderBegin.renderPass = mRenderPass;
		renderBegin.framebuffer = framebuffer;
		renderBegin.renderArea.offset = { 0, 0 };
		renderBegin.renderArea.extent = mExtent;
		renderBegin.clearValueCount = clearValues.size();
		renderBegin.pClearValues = clearValues.data();

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBeginRenderPass(mCommandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		VkDeviceSize offsets[] = { 0 };
		
		for (const auto& renderable : mRenderables) {
			vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &renderable.vbo.memoryResource, offsets);
			vkCmdBindIndexBuffer(mCommandBuffer, renderable.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(
				mCommandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mPipelineLayout, 
				0, 
				1, 
				&renderable.descriptorSet, 
				0, 
				nullptr);
			vkCmdDrawIndexed(mCommandBuffer, renderable.numIndices, 1, 0, 0, 0);
		}

		if (sDrawNormals) {
			vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mNormalsPipeline);
			for (const auto& renderable : mRenderables) {
				vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &renderable.normalVbo.memoryResource, offsets);
				vkCmdBindDescriptorSets(
					mCommandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					mPipelineLayout,
					0,
					1,
					&renderable.descriptorSet,
					0,
					nullptr);
				vkCmdDraw(mCommandBuffer, renderable.numNormalVertices, 1, 0, 0);
			}
		}

		ImGuiIO& io = ImGui::GetIO();
		ImDrawData* imDrawData = ImGui::GetDrawData();

		if (imDrawData->CmdListsCount > 0) {
			vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mUiPipeline);
			vkCmdBindDescriptorSets(
				mCommandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mUiPipelineLayout, 
				0, 
				1, 
				&mUiDescriptorSet, 
				0, 
				nullptr);
			UiPushConstant push = {};
			push.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
			push.pos = glm::vec2(-1.f);
			vkCmdPushConstants(mCommandBuffer, mUiPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UiPushConstant), &push);
			VkDeviceSize offsets[1] = { 0 };

			// Create vertex buffer
			uint32_t vertexMemorySize = sizeof(ImDrawVert) * imDrawData->TotalVtxCount;
			uint32_t indexMemorySize = sizeof(ImDrawIdx) * imDrawData->TotalIdxCount;
			if (vertexMemorySize && indexMemorySize) {
				if (mUiVbo.allocationInfo.size < vertexMemorySize) {
					if (mUiVbo.allocationInfo.pMappedData != nullptr) {
						vmaDestroyBuffer(mAllocator, mUiVbo.memoryResource, mUiVbo.allocation);
					}
					VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; bufferInfo.size = vertexMemorySize;
					bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

					VmaAllocationCreateInfo allocCreateInfo = {};
					allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
					allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
					vmaCreateBuffer(
						mAllocator,
						&bufferInfo,
						&allocCreateInfo,
						&mUiVbo.memoryResource,
						&mUiVbo.allocation,
						&mUiVbo.allocationInfo);

				}

				if (mUiIbo.allocationInfo.size < indexMemorySize) {
					if (mUiIbo.allocationInfo.pMappedData != nullptr) {
						vmaDestroyBuffer(mAllocator, mUiIbo.memoryResource, mUiIbo.allocation);
					}
					VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
					iboInfo.size = indexMemorySize;
					iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

					VmaAllocationCreateInfo indexAllocCreateInfo = {};
					indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
					indexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
					vmaCreateBuffer(
						mAllocator,
						&iboInfo,
						&indexAllocCreateInfo,
						&mUiIbo.memoryResource,
						&mUiIbo.allocation,
						&mUiIbo.allocationInfo);

				}

				ImDrawVert* vertDst = static_cast<ImDrawVert*>(mUiVbo.allocationInfo.pMappedData);
				ImDrawIdx* indexDst = static_cast<ImDrawIdx*>(mUiIbo.allocationInfo.pMappedData);

				for (int i = 0; i < imDrawData->CmdListsCount; ++i) {
					const ImDrawList* cmdList = imDrawData->CmdLists[i];
					memcpy(vertDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
					memcpy(indexDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
					vertDst += cmdList->VtxBuffer.Size;
					indexDst += cmdList->IdxBuffer.Size;
				}

				vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mUiVbo.memoryResource, offsets);
				vkCmdBindIndexBuffer(mCommandBuffer, mUiIbo.memoryResource, 0, VK_INDEX_TYPE_UINT16);

				int vertexOffset = 0;
				int indexOffset = 0;
				for (int i = 0; i < imDrawData->CmdListsCount; ++i) {
					const ImDrawList* cmdList = imDrawData->CmdLists[i];
					for (int j = 0; j < cmdList->CmdBuffer.Size; ++j) {
						const ImDrawCmd* cmd = &cmdList->CmdBuffer[j];
						vkCmdDrawIndexed(mCommandBuffer, cmd->ElemCount, 1, indexOffset, vertexOffset, 0);
						indexOffset += cmd->ElemCount;
					}
					vertexOffset += cmdList->VtxBuffer.Size;
				}
			}
		}

		vkCmdEndRenderPass(mCommandBuffer);
		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);
	}

	VkSemaphore Renderer::drawFrame(VkFramebuffer& framebuffer, VkSemaphore* waitSemaphores, uint32_t waitSemaphoreCount) {
		ImGui::NewFrame();
		// update uniforms
		for (const auto& renderable : mRenderables) {
			UniformBufferObject ubo = {};
			ubo.model = renderable.renderObject->getWorldTransform();
			ubo.view = mCamera->getViewTransform();
			ubo.modelViewProj = mCamera->getProjection() * ubo.view * ubo.model;
			//ubo.modelViewProj[1][1] *= -1;
			memcpy(renderable.ubo.allocationInfo.pMappedData, &ubo, sizeof(ubo));
		}

		ImGui::Text("Text Blah");
		ImGui::ShowDemoWindow();

		ImGui::EndFrame();
		ImGui::Render();

		// record the command buffer
		recordCommandBuffer(framebuffer);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.waitSemaphoreCount = waitSemaphoreCount;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mCommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &mRenderFinished;

		assert(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mRenderFence) == VK_SUCCESS);

		// caller can determine when render is finished for present
		return mRenderFinished;
	}
}
