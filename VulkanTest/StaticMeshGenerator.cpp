#include "StaticMeshGenerator.h"
//#define HVK_UTIL_IMPLEMENTATION
#include "vulkan-util.h"

namespace hvk
{
	StaticMeshGenerator::StaticMeshGenerator(
		VulkanDevice device, 
		VmaAllocator allocator, 
		VkQueue graphicsQueue, 
		VkRenderPass renderPass,
		VkCommandPool commandPool) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mLightsDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mLightsDescriptorSet(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo(),
		mLightsUbo(),
		mRenderables(),
		mLights(),
        mGammaCorrection(2.2f),
        mUseSRGBTex(false)
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

		VkDescriptorSetLayoutBinding diffuseSamplerBinding = {};
		diffuseSamplerBinding.binding = 1;
		diffuseSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		diffuseSamplerBinding.descriptorCount = 1;
		diffuseSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		diffuseSamplerBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding metalRoughSamplerBinding = {};
		metalRoughSamplerBinding.binding = 2;
		metalRoughSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		metalRoughSamplerBinding.descriptorCount = 1;
		metalRoughSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		metalRoughSamplerBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding normalSamplerBinding = {};
		normalSamplerBinding.binding = 3;
		normalSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalSamplerBinding.descriptorCount = 1;
		normalSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		normalSamplerBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { 
            uboLayoutBinding, 
            diffuseSamplerBinding,
            metalRoughSamplerBinding,
            normalSamplerBinding
        };
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		assert(vkCreateDescriptorSetLayout(mDevice.device, &layoutInfo, nullptr, &mDescriptorSetLayout) == VK_SUCCESS);

		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = MAX_UBOS;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = MAX_SAMPLERS;

		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
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

    StaticMeshGenerator::~StaticMeshGenerator()
    {
        for (auto& renderable : mRenderables)
        {
            // destroy buffers
            vmaDestroyBuffer(mAllocator, renderable.vbo.memoryResource, renderable.vbo.allocation);
            vmaDestroyBuffer(mAllocator, renderable.ibo.memoryResource, renderable.ibo.allocation);
            vmaDestroyBuffer(mAllocator, renderable.ubo.memoryResource, renderable.ubo.allocation);

            // destroy textures
			destroyMap(mDevice.device, mAllocator, renderable.diffuseMap);
			destroyMap(mDevice.device, mAllocator, renderable.metallicRoughnessMap);
			destroyMap(mDevice.device, mAllocator, renderable.normalMap);
        }

        vmaDestroyBuffer(mAllocator, mLightsUbo.memoryResource, mLightsUbo.allocation);
        vkDestroyDescriptorSetLayout(mDevice.device, mLightsDescriptorSetLayout, nullptr);

        vkDestroyDescriptorSetLayout(mDevice.device, mDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(mDevice.device, mDescriptorPool, nullptr);

        vkDestroyPipeline(mDevice.device, mPipeline, nullptr);
        vkDestroyPipelineLayout(mDevice.device, mPipelineInfo.pipelineLayout, nullptr);
    }

	void StaticMeshGenerator::preparePipelineInfo()
	{
		VkPushConstantRange pushRange = {};
		pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushRange.size = sizeof(hvk::PushConstant);
		pushRange.offset = 0;

		std::array<VkDescriptorSetLayout, 2> dsLayouts = { mLightsDescriptorSetLayout, mDescriptorSetLayout };
		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = static_cast<uint32_t>(dsLayouts.size());
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

		const StaticMesh::Vertices vertices = object->getVertices();
		const StaticMesh::Indices indices = object->getIndices();
		newRenderable.numVertices = static_cast<uint32_t>(vertices->size());
		newRenderable.numIndices = static_cast<uint32_t>(indices->size());

		// Create vertex buffer
        size_t vertexMemorySize = sizeof(Vertex) * newRenderable.numVertices;
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

        memcpy(newRenderable.vbo.allocationInfo.pMappedData, vertices->data(), vertexMemorySize);

		// Create index buffer
        size_t indexMemorySize = sizeof(uint16_t) * newRenderable.numIndices;
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

        memcpy(newRenderable.ibo.allocationInfo.pMappedData, indices->data(), indexMemorySize);

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
		std::shared_ptr<Material> mat = object->getMaterial();
		if (mat->diffuseProp.texture != nullptr) {
			const tinygltf::Image& diffuseTex = *mat->diffuseProp.texture;
			newRenderable.diffuseMap.texture = createTextureImage(
				mDevice.device,
				mAllocator,
				mCommandPool,
				mGraphicsQueue,
				diffuseTex.image.data(),
				1,
				diffuseTex.width,
				diffuseTex.height,
				diffuseTex.component * (diffuseTex.bits / 8));
			newRenderable.diffuseMap.view = createImageView(
                mDevice.device, 
                newRenderable.diffuseMap.texture.memoryResource, 
                VK_FORMAT_R8G8B8A8_UNORM);
			newRenderable.diffuseMap.sampler = createTextureSampler(mDevice.device);
		}

        if (mat->metallicRoughnessProp.texture != nullptr)
        {
			const tinygltf::Image& metRoughTex = *mat->metallicRoughnessProp.texture;
			newRenderable.metallicRoughnessMap.texture = createTextureImage(
				mDevice.device,
				mAllocator,
				mCommandPool,
				mGraphicsQueue,
				metRoughTex.image.data(),
				1,
				metRoughTex.width,
				metRoughTex.height,
				metRoughTex.component * (metRoughTex.bits / 8));
			newRenderable.metallicRoughnessMap.view = createImageView(
                mDevice.device, 
                newRenderable.metallicRoughnessMap.texture.memoryResource, 
                VK_FORMAT_R8G8B8A8_UNORM);
			newRenderable.metallicRoughnessMap.sampler = createTextureSampler(mDevice.device);
        }

        if (mat->normalProp.texture != nullptr)
        {
			const tinygltf::Image& normalTex = *mat->normalProp.texture;
			newRenderable.normalMap.texture = createTextureImage(
				mDevice.device,
				mAllocator,
				mCommandPool,
				mGraphicsQueue,
				normalTex.image.data(),
				1,
				normalTex.width,
				normalTex.height,
				normalTex.component * (normalTex.bits / 8));
			newRenderable.normalMap.view = createImageView(
                mDevice.device, 
                newRenderable.normalMap.texture.memoryResource, 
                VK_FORMAT_R8G8B8A8_UNORM);
			newRenderable.normalMap.sampler = createTextureSampler(mDevice.device);
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
			imageInfo.imageView = newRenderable.diffuseMap.view;
			imageInfo.sampler = newRenderable.diffuseMap.sampler;

			VkDescriptorImageInfo metallicRoughnessInfo = {};
			metallicRoughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			metallicRoughnessInfo.imageView = newRenderable.metallicRoughnessMap.view;
			metallicRoughnessInfo.sampler = newRenderable.metallicRoughnessMap.sampler;

			VkDescriptorImageInfo normalInfo = {};
			normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			normalInfo.imageView = newRenderable.normalMap.view;
			normalInfo.sampler = newRenderable.normalMap.sampler;

			VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrite.dstSet = newRenderable.descriptorSet;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			VkWriteDescriptorSet imageDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			imageDescriptorWrite.dstSet = newRenderable.descriptorSet;
			imageDescriptorWrite.dstBinding = 1;
			imageDescriptorWrite.dstArrayElement = 0;
			imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageDescriptorWrite.descriptorCount = 1;
			imageDescriptorWrite.pImageInfo = &imageInfo;

			VkWriteDescriptorSet mtlRoughDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			mtlRoughDescriptorWrite.dstSet = newRenderable.descriptorSet;
			mtlRoughDescriptorWrite.dstBinding = 2;
			mtlRoughDescriptorWrite.dstArrayElement = 0;
			mtlRoughDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			mtlRoughDescriptorWrite.descriptorCount = 1;
			mtlRoughDescriptorWrite.pImageInfo = &metallicRoughnessInfo;

			VkWriteDescriptorSet normalDescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			normalDescriptorWrite.dstSet = newRenderable.descriptorSet;
			normalDescriptorWrite.dstBinding = 3;
			normalDescriptorWrite.dstArrayElement = 0;
			normalDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			normalDescriptorWrite.descriptorCount = 1;
			normalDescriptorWrite.pImageInfo = &normalInfo;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites = {
				descriptorWrite,
				imageDescriptorWrite,
				mtlRoughDescriptorWrite,
				normalDescriptorWrite
			};

			vkUpdateDescriptorSets(mDevice.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		mRenderables.push_back(newRenderable);
	}

	void StaticMeshGenerator::addLight(std::shared_ptr<Light> light)
	{
		mLights.push_back(light);
	}

	VkCommandBuffer& StaticMeshGenerator::drawFrame(
        const VkCommandBufferInheritanceInfo& inheritance,
		const VkFramebuffer& framebuffer,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const Camera& camera,
		const AmbientLight& ambientLight)
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
		uboLights.numLights = static_cast<uint32_t>(mLights.size());
        uboLights.ambient = ambientLight;
		for (size_t i = 0; i < mLights.size(); ++i) {
			HVK_shared<Light> light = mLights[i];
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

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBegin.pInheritanceInfo = &inheritance;

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
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
			const Material& mat = *renderable.renderObject->getMaterial();
            push.gamma = mGammaCorrection;
            push.sRGBTextures = mUseSRGBTex;
			vkCmdPushConstants(mCommandBuffer, mPipelineInfo.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &push);
			vkCmdDrawIndexed(mCommandBuffer, renderable.numIndices, 1, 0, 0, 0);
		}

		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}

    void StaticMeshGenerator::setGammaCorrection(float gamma)
    {
        mGammaCorrection = gamma;
    }

    void StaticMeshGenerator::setUseSRGBTex(bool useSRGBTex)
    {
        mUseSRGBTex = useSRGBTex;
    }
}