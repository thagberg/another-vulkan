<<<<<<< HEAD
=======
#include <assert.h>

#include "imgui/imgui.h"

>>>>>>> 5e9c104... Implementing UI renderer
#include "DrawlistGenerator.h"

namespace hvk
{

    DrawlistGenerator::DrawlistGenerator()
    {
    }


    DrawlistGenerator::~DrawlistGenerator()
    {
    }

	StaticMeshGenerator::StaticMeshGenerator(
		VulkanDevice device, 
		VmaAllocator allocator, 
		VkQueue graphicsQueue, 
		VkRenderPass renderPass) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mLightsDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mLightsDescriptorSet(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo(),
		mLightsUbo(),
		mRenderables(),
		mLights()
	{
		mRenderables.reserve(NUM_INITIAL_RENDEROBJECTS);
		mLights.reserve(NUM_INITIAL_LIGHTS);

		/***************
		 Create descriptor set layout and descriptor pool
		***************/
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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
		poolSizes[0].descriptorCount = MAX_UBOS;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = MAX_SAMPLERS;

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = MAX_DESCRIPTORS;

		assert(vkCreateDescriptorPool(mDevice.device, &poolInfo, nullptr, &mDescriptorPool) == VK_SUCCESS);

		/*************
		 Create Lights UBO
		 *************/
		uint32_t uboMemorySize = sizeof(hvk::UniformLightObject<NUM_INITIAL_LIGHTS>);
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
			&mLightsUbo.memoryResource,
			&mLightsUbo.allocation,
			&mLightsUbo.allocationInfo);

		/*****************
		 Create Lights descriptor set
		******************/
		VkDescriptorSetLayoutBinding lightLayoutBinding = {};
		lightLayoutBinding.binding = 0;
		lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightLayoutBinding.descriptorCount = 1;
		lightLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		lightLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo lightsLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		lightsLayoutInfo.bindingCount = 1;
		lightsLayoutInfo.pBindings = &lightLayoutBinding;

		assert(vkCreateDescriptorSetLayout(mDevice.device, &lightsLayoutInfo, nullptr, &mLightsDescriptorSetLayout) == VK_SUCCESS);

		VkDescriptorSetAllocateInfo lightsAlloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		lightsAlloc.descriptorPool = mDescriptorPool;
		lightsAlloc.descriptorSetCount = 1;
		lightsAlloc.pSetLayouts = &mLightsDescriptorSetLayout;

		assert(vkAllocateDescriptorSets(mDevice.device, &lightsAlloc, &mLightsDescriptorSet) == VK_SUCCESS);

		VkDescriptorBufferInfo lightsBufferInfo = {};
		lightsBufferInfo.buffer = mLightsUbo.memoryResource;
		lightsBufferInfo.offset = 0;
		lightsBufferInfo.range = NUM_INITIAL_LIGHTS * sizeof(hvk::UniformLight);

		VkWriteDescriptorSet lightsDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		lightsDescriptorWrite.dstSet = mLightsDescriptorSet;
		lightsDescriptorWrite.dstBinding = 0;
		lightsDescriptorWrite.dstArrayElement = 0;
		lightsDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightsDescriptorWrite.descriptorCount = 1;
		lightsDescriptorWrite.pBufferInfo = &lightsBufferInfo;

		vkUpdateDescriptorSets(mDevice.device, 1, &lightsDescriptorWrite, 0, nullptr);

		/*
		 prepare graphics pipeline info	
		*/
		preparePipelineInfo();

		// initialized
		setInitialized(true);
	}

	void StaticMeshGenerator::preparePipelineInfo()
	{
		VkPushConstantRange pushRange = {};
		pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushRange.size = sizeof(hvk::PushConstant);
		pushRange.offset = 0;

		std::array<VkDescriptorSetLayout, 2> dsLayouts = { mLightsDescriptorSetLayout, mDescriptorSetLayout };
		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = dsLayouts.size();
		layoutCreate.pSetLayouts = dsLayouts.data();
		layoutCreate.pushConstantRangeCount = 1;
		layoutCreate.pPushConstantRanges = &pushRange;

		assert(vkCreatePipelineLayout(mDevice.device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		mPipelineInfo.vertexInfo = { };
		mPipelineInfo.vertexInfo.bindingDescription = hvk::Vertex::getBindingDescription();
		mPipelineInfo.vertexInfo.attributeDescriptions = hvk::Vertex::getAttributeDescriptions();
		mPipelineInfo.vertexInfo.vertexInputInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			0,
			1,
			&mPipelineInfo.vertexInfo.bindingDescription,
			static_cast<uint32_t>(mPipelineInfo.vertexInfo.attributeDescriptions.size()),
			mPipelineInfo.vertexInfo.attributeDescriptions.data()
		};

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		mPipelineInfo.blendAttachments = { blendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = "shaders/compiled/vert.spv";
		mPipelineInfo.fragShaderFile = "shaders/compiled/frag.spv";
		mPipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		mPipeline = generatePipeline(mDevice, mRenderPass, mPipelineInfo);
	}

	void StaticMeshGenerator::updateRenderPass(VkRenderPass renderPass)
	{
		mRenderPass = renderPass;
		mPipeline = generatePipeline(mDevice, mRenderPass, mPipelineInfo);
		setInitialized(true);
	}

	void StaticMeshGenerator::invalidate()
	{
		setInitialized(false);
		vkDestroyPipeline(mDevice.device, mPipeline, nullptr);
	}

	void StaticMeshGenerator::addStaticMeshObject(std::shared_ptr<StaticMeshRenderObject> object)
	{
		StaticMeshRenderable newRenderable;
		newRenderable.renderObject = object;

		const std::vector<Vertex>& vertices = object->getVertices();
		const std::vector<VertIndex>& indices = object->getIndices();
		newRenderable.numVertices = vertices.size();
		newRenderable.numIndices = indices.size();

		// Create vertex buffer
        uint32_t vertexMemorySize = sizeof(Vertex) * vertices.size();
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

        memcpy(newRenderable.vbo.allocationInfo.pMappedData, vertices.data(), (size_t)vertexMemorySize);

		// Create index buffer
        uint32_t indexMemorySize = sizeof(uint16_t) * indices.size();
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

        memcpy(newRenderable.ibo.allocationInfo.pMappedData, indices.data(), (size_t)indexMemorySize);

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

		// create texture view and sampler
		const Material& mat = object->getMaterial();
		if (mat.diffuseProp.texture != nullptr) {
			const tinygltf::Image& diffuseTex = *mat.diffuseProp.texture;
			newRenderable.texture = createTextureImage(
				mDevice.device,
				mAllocator,
				mCommandPool,
				mGraphicsQueue,
				diffuseTex.image.data(),
				diffuseTex.width,
				diffuseTex.height,
				diffuseTex.component * (diffuseTex.bits / 8));
			newRenderable.textureView = createImageView(mDevice.device, newRenderable.texture.memoryResource, VK_FORMAT_R8G8B8A8_UNORM);
			newRenderable.textureSampler = createTextureSampler(mDevice.device);
		}

		// TODO: pre-allocate a number of descriptor sets for renderables
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

		mRenderables.push_back(newRenderable);
	}

	void StaticMeshGenerator::addLight(std::shared_ptr<Light> light)
	{
		mLights.push_back(light);
	}

	VkCommandBuffer& StaticMeshGenerator::drawFrame(
		const VkFramebuffer& framebuffer,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const Camera& camera,
		const AmbientLight& ambientLight,
		VkFence waitFence)
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

		/*****************
		 update lights UBO
		*****************/
		int memOffset = 0;
		auto* copyaddr = reinterpret_cast<UniformLightObject<NUM_INITIAL_LIGHTS>*>(mLightsUbo.allocationInfo.pMappedData);
		auto uboLights = UniformLightObject<NUM_INITIAL_LIGHTS>();
		uboLights.numLights = mLights.size();
        uboLights.ambient = ambientLight;
		for (size_t i = 0; i < mLights.size(); ++i) {
			LightRef light = mLights[i];
			UniformLight ubo = {};
			ubo.lightPos = light->getWorldPosition();
			ubo.lightColor = light->getColor();
			ubo.lightIntensity = light->getIntensity();
			uboLights.lights[i] = ubo;
		}
		memcpy(copyaddr, &uboLights, sizeof(uboLights));

		/******************
		 record commands
		******************/
		//assert(vkWaitForFences(mDevice.device, 1, &waitFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		commandBegin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBegin.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderBegin.renderPass = mRenderPass;
		renderBegin.framebuffer = framebuffer;
		renderBegin.renderArea = scissor;
		renderBegin.clearValueCount = clearValues.size();
		renderBegin.pClearValues = clearValues.data();

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBeginRenderPass(mCommandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		// bind viewport and scissor
		vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		// bind lights descriptor set to set 0
		vkCmdBindDescriptorSets(
			mCommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			mPipelineInfo.pipelineLayout,
			0,
			1,
			&mLightsDescriptorSet,
			0,
			nullptr);

		// draw renderables
		for (const auto& renderable : mRenderables) {
			vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &renderable.vbo.memoryResource, offsets);
			vkCmdBindIndexBuffer(mCommandBuffer, renderable.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(
				mCommandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mPipelineInfo.pipelineLayout, 
				1, 
				1,
				&renderable.descriptorSet,
				0, 
				nullptr);

			PushConstant push = {};
			const Material& mat = renderable.renderObject->getMaterial();
			push.specular = mat.specularProp.scale;
			push.shininess = 1.f - mat.roughnessProp.scale;
			vkCmdPushConstants(mCommandBuffer, mPipelineInfo.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &push);
			vkCmdDrawIndexed(mCommandBuffer, renderable.numIndices, 1, 0, 0, 0);
		}

		assert(vkResetFences(mDevice.device, 1, &waitFence) == VK_SUCCESS);
		vkCmdEndRenderPass(mCommandBuffer);
		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}

	StaticMeshGenerator::~StaticMeshGenerator()
	{

	}

	UiDrawGenerator::UiDrawGenerator(
		VulkanDevice device,
		VmaAllocator allocator,
		VkQueue graphicsQueue,
		VkRenderPass renderPass,
		VkExtent2D windowExtent) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mDescriptorSet(VK_NULL_HANDLE),
		mFontView(VK_NULL_HANDLE),
		mFontSampler(VK_NULL_HANDLE),
		mVbo(),
		mIbo(),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo(),
		mWindowExtent(windowExtent)
	{
		// Initialize Imgui stuff
		unsigned char* fontTextureData;
		int fontTextWidth, fontTextHeight, bytesPerPixel;
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();
		io.Fonts->GetTexDataAsRGBA32(&fontTextureData, &fontTextWidth, &fontTextHeight, &bytesPerPixel);
		io.DisplaySize = ImVec2(mWindowExtent.width, mWindowExtent.height);
		io.DisplayFramebufferScale = ImVec2(1.f, 1.f);

        Resource<VkImage> uiFont = createTextureImage(
			mDevice.device, 
			mAllocator, 
			mCommandPool, 
			mGraphicsQueue,
			fontTextureData,
			fontTextWidth,
			fontTextHeight,
			bytesPerPixel);

		/***************
		 Create descriptor set layout and descriptor pool
		***************/
		std::array<VkDescriptorPoolSize, 1> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = MAX_SAMPLERS;

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = MAX_DESCRIPTORS;

		assert(vkCreateDescriptorPool(mDevice.device, &poolInfo, nullptr, &mDescriptorPool) == VK_SUCCESS);

		VkDescriptorSetLayout uiDescriptorSetLayout;
		mFontView = createImageView(mDevice.device, uiFont.memoryResource, VK_FORMAT_R8G8B8A8_UNORM);
		mFontSampler = createTextureSampler(mDevice.device);

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

		assert(vkAllocateDescriptorSets(mDevice.device, &uiAlloc, &mDescriptorSet) == VK_SUCCESS);

		VkDescriptorImageInfo fontImageInfo = {};
		fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		fontImageInfo.imageView = mFontView;
		fontImageInfo.sampler = mFontSampler;

		VkWriteDescriptorSet fontDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		fontDescriptorWrite.dstSet = mDescriptorSet;
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

		assert(vkCreatePipelineLayout(mDevice.device, &uiLayoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		mPipelineInfo.vertexInfo = {};
		mPipelineInfo.vertexInfo.bindingDescription = {};
		mPipelineInfo.vertexInfo.bindingDescription.binding = 0;
		mPipelineInfo.vertexInfo.bindingDescription.stride = sizeof(ImDrawVert);
		mPipelineInfo.vertexInfo.bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		mPipelineInfo.vertexInfo.attributeDescriptions.resize(3);
		mPipelineInfo.vertexInfo.attributeDescriptions[0] = {
			0,							// location
			0,							// binding
			UiVertexPositionFormat,		// format
			offsetof(ImDrawVert, pos)	// offset
			};

		mPipelineInfo.vertexInfo.attributeDescriptions[1] = {
			1,
			0,
			UiVertexUVFormat,
			offsetof(ImDrawVert, uv)
			};

		mPipelineInfo.vertexInfo.attributeDescriptions[2] = {
			2,
			0,
			UiVertexColorFormat,
			offsetof(ImDrawVert, col)
			};

		mPipelineInfo.vertexInfo.vertexInputInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			0,
			1,
			&mPipelineInfo.vertexInfo.bindingDescription,
			static_cast<uint32_t>(mPipelineInfo.vertexInfo.attributeDescriptions.size()),
			mPipelineInfo.vertexInfo.attributeDescriptions.data()
		};

		VkPipelineColorBlendAttachmentState uiBlendAttachment = {};
		uiBlendAttachment.blendEnable = VK_TRUE;
		uiBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		uiBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		uiBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		uiBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		uiBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		uiBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		uiBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		mPipelineInfo.blendAttachments = { uiBlendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = "shaders/compiled/ui_v.spv";
		mPipelineInfo.fragShaderFile = "shaders/compiled/ui_f.spv";
		mPipelineInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

		mPipeline = generatePipeline(mDevice, mRenderPass, mPipelineInfo);

		setInitialized(true);
	}

	UiDrawGenerator::~UiDrawGenerator()
	{

	}

	void UiDrawGenerator::invalidate()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void UiDrawGenerator::updateRenderPass(VkRenderPass renderPass, VkExtent2D windowExtent)
	{
		mWindowExtent = windowExtent;
		mRenderPass = renderPass;
		mPipeline = generatePipeline(mDevice, mRenderPass, mPipelineInfo);

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(mWindowExtent.width, mWindowExtent.height);
		io.DisplayFramebufferScale = ImVec2(1.f, 1.f);

		setInitialized(true);
	}

	VkCommandBuffer& UiDrawGenerator::drawFrame(
		VkFramebuffer& framebuffer,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		VkFence waitFence)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::EndFrame();
		ImGui::Render();

		//assert(vkWaitForFences(mDevice.device, 1, &waitFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);
		//assert(vkResetFences(mDevice.device, 1, &waitFence) == VK_SUCCESS);

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		commandBegin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBegin.pInheritanceInfo = nullptr;

		VkRenderPassBeginInfo renderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderBegin.renderPass = mRenderPass;
		renderBegin.framebuffer = framebuffer;
		renderBegin.renderArea = scissor;
		renderBegin.clearValueCount = clearValues.size();
		renderBegin.pClearValues = clearValues.data();

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
		vkCmdBeginRenderPass(mCommandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		// bind viewport and scissor
		vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		ImDrawData* imDrawData = ImGui::GetDrawData();
		if (imDrawData->CmdListsCount > 0) {
			vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
			vkCmdBindDescriptorSets(
				mCommandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				mPipelineInfo.pipelineLayout, 
				0, 
				1, 
				&mDescriptorSet, 
				0, 
				nullptr);
			UiPushConstant push = {};
			push.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
			push.pos = glm::vec2(-1.f);
			vkCmdPushConstants(mCommandBuffer, mPipelineInfo.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UiPushConstant), &push);
			VkDeviceSize offsets[1] = { 0 };

			// Create vertex buffer
			uint32_t vertexMemorySize = sizeof(ImDrawVert) * imDrawData->TotalVtxCount;
			uint32_t indexMemorySize = sizeof(ImDrawIdx) * imDrawData->TotalIdxCount;
			if (vertexMemorySize && indexMemorySize) {
				if (mVbo.allocationInfo.size < vertexMemorySize) {
					if (mVbo.allocationInfo.pMappedData != nullptr) {
						vmaDestroyBuffer(mAllocator, mVbo.memoryResource, mVbo.allocation);
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
						&mVbo.memoryResource,
						&mVbo.allocation,
						&mVbo.allocationInfo);

				}

				if (mIbo.allocationInfo.size < indexMemorySize) {
					if (mIbo.allocationInfo.pMappedData != nullptr) {
						vmaDestroyBuffer(mAllocator, mIbo.memoryResource, mIbo.allocation);
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
						&mIbo.memoryResource,
						&mIbo.allocation,
						&mIbo.allocationInfo);

				}

				ImDrawVert* vertDst = static_cast<ImDrawVert*>(mVbo.allocationInfo.pMappedData);
				ImDrawIdx* indexDst = static_cast<ImDrawIdx*>(mIbo.allocationInfo.pMappedData);

				for (int i = 0; i < imDrawData->CmdListsCount; ++i) {
					const ImDrawList* cmdList = imDrawData->CmdLists[i];
					memcpy(vertDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
					memcpy(indexDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
					vertDst += cmdList->VtxBuffer.Size;
					indexDst += cmdList->IdxBuffer.Size;
				}

				vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mVbo.memoryResource, offsets);
				vkCmdBindIndexBuffer(mCommandBuffer, mIbo.memoryResource, 0, VK_INDEX_TYPE_UINT16);

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

		return mCommandBuffer;
	}

}
