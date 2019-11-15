#include "SkyboxGenerator.h"

#include "stb_image.h"

#include "vulkan-util.h"
#include "ResourceManager.h"
#include "shapes.h"


namespace hvk
{
	SkyboxGenerator:: SkyboxGenerator(
		VulkanDevice device, 
		VmaAllocator allocator, 
		VkQueue graphicsQueue, 
		VkRenderPass renderPass, 
		VkCommandPool commandPool) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mDescriptorSet(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo(),
		mSkyboxMap(),
		mSkyboxRenderable()
	{
		auto cube = createColoredCube(glm::vec3(0.f, 1.f, 0.f));
		mSkyboxRenderable.renderObject = HVK_make_shared<DebugMeshRenderObject>(
			"Sky Box",
			nullptr,
			glm::mat4(1.f),
			cube);

        // load skybox textures
        /*
        
                T
            |L |F |R |BA
                BO
        
        */
        std::array<std::string, 6> skyboxFiles = {
            "resources/sky/desertsky_lf.tga",
            "resources/sky/desertsky_ft.tga",
            "resources/sky/desertsky_up.tga",
            "resources/sky/desertsky_dn.tga",
            "resources/sky/desertsky_rt.tga",
            "resources/sky/desertsky_bk.tga"
        };

        int width, height, numChannels;
		int copyOffset = 0;
		int copySize = 0;
		int layerSize = 0;
		unsigned char* copyTo = nullptr;
		unsigned char* layers[6];
        for (size_t i = 0; i < skyboxFiles.size(); ++i)
        {
            unsigned char* data = stbi_load(
                skyboxFiles[i].c_str(), 
                &width, 
                &height, 
                &numChannels, 
                0);

			assert(data != nullptr);

			layers[i] = data;
			copySize += width * height * numChannels;
        }
		layerSize = copySize / skyboxFiles.size();

		copyTo = static_cast<unsigned char*>(ResourceManager::alloc(copySize, alignof(unsigned char)));
		for (size_t i = 0; i < 6; ++i)
		{
			void* dst = copyTo + copyOffset;
			copyOffset += layerSize;
			memcpy(dst, layers[i], layerSize);
			stbi_image_free(layers[i]);
		}

		mSkyboxMap.texture = createTextureImage(
			mDevice.device, 
			mAllocator, 
			mCommandPool, 
			mGraphicsQueue, 
			copyTo, 
			6, 
			width, 
			height, 
			numChannels);
		mSkyboxMap.view = createImageView(
			mDevice.device,
			mSkyboxMap.texture.memoryResource,
			VK_FORMAT_R8G8B8A8_UNORM);
		mSkyboxMap.sampler = createTextureSampler(mDevice.device);

		ResourceManager::free(copyTo, copySize);

		auto skyboxMesh = createColoredCube(glm::vec3(0.1f, 4.f, 1.f));
		auto vertices = skyboxMesh->getVertices();
		auto indices = skyboxMesh->getIndices();
		mSkyboxRenderable.numVertices = static_cast<uint32_t>(vertices->size());
		mSkyboxRenderable.numIndices = static_cast<uint32_t>(indices->size());

		// Create VBO
		size_t vertexMemorySize = sizeof(ColorVertex) * mSkyboxRenderable.numVertices;
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
			&mSkyboxRenderable.vbo.memoryResource,
			&mSkyboxRenderable.vbo.allocation,
			&mSkyboxRenderable.vbo.allocationInfo);

		memcpy(mSkyboxRenderable.vbo.allocationInfo.pMappedData, vertices->data(), vertexMemorySize);

		// Create IBO
        size_t indexMemorySize = sizeof(uint16_t) * mSkyboxRenderable.numIndices;
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
            &mSkyboxRenderable.ibo.memoryResource,
            &mSkyboxRenderable.ibo.allocation,
            &mSkyboxRenderable.ibo.allocationInfo);

        memcpy(mSkyboxRenderable.ibo.allocationInfo.pMappedData, indices->data(), indexMemorySize);

		// Create UBO
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
			&mSkyboxRenderable.ubo.memoryResource,
			&mSkyboxRenderable.ubo.allocation,
			&mSkyboxRenderable.ubo.allocationInfo);

		// Create descriptor pool
		std::vector<VkDescriptorPoolSize> poolSizes(2, VkDescriptorPoolSize{});
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = 1;

		createDescriptorPool(mDevice.device, poolSizes, 1, mDescriptorPool);

		// Create descriptor set layout
		auto uboLayoutBinding = generateUboLayoutBinding(0, 1);
		auto skySamplerBinding = generateSamplerLayoutBinding(1, 1);

		std::vector<decltype(uboLayoutBinding)> bindings = {
			uboLayoutBinding,
			skySamplerBinding
		};
		createDescriptorSetLayout(mDevice.device, bindings, mDescriptorSetLayout);

		// Create descriptor set
		std::vector<VkDescriptorSetLayout> layouts = { mDescriptorSetLayout };
		allocateDescriptorSets(mDevice.device, mDescriptorPool, mDescriptorSet, layouts);

		// Update descriptor set
		VkDescriptorBufferInfo dsBufferInfo = {
			mSkyboxRenderable.ubo.memoryResource,
			0,
			sizeof(hvk::UniformBufferObject)
		};

		VkDescriptorImageInfo imageInfo = {
			mSkyboxMap.sampler,
			mSkyboxMap.view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkWriteDescriptorSet bufferWrite = {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			mDescriptorSet,
			0,
			0,
			1,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			nullptr,
			&dsBufferInfo,
			nullptr
		};

		std::vector<VkWriteDescriptorSet> descriptorWrites = {
			bufferWrite
		};
		vkUpdateDescriptorSets(
			mDevice.device, 
			static_cast<uint32_t>(descriptorWrites.size()), 
			descriptorWrites.data(), 
			0, 
			nullptr);


		// Prepare pipeline
		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = static_cast<uint32_t>(layouts.size());
		layoutCreate.pSetLayouts = layouts.data();
		layoutCreate.pushConstantRangeCount = 0;
		layoutCreate.pPushConstantRanges = nullptr;
		assert(vkCreatePipelineLayout(mDevice.device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		mPipelineInfo.vertexInfo = { };
		mPipelineInfo.vertexInfo.bindingDescription = hvk::ColorVertex::getBindingDescription();
		mPipelineInfo.vertexInfo.attributeDescriptions = hvk::ColorVertex::getAttributeDescriptions();
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
		mPipelineInfo.vertShaderFile = "shaders/compiled/sky_vert.spv";
		mPipelineInfo.fragShaderFile = "shaders/compiled/sky_frag.spv";
		mPipelineInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

		mPipelineInfo.depthStencilState = createDepthStencilState(false, false);

		mPipeline = generatePipeline(mDevice, mRenderPass, mPipelineInfo);
		

		setInitialized(true);
	}

	void SkyboxGenerator::updateRenderPass(VkRenderPass renderPass)
	{
		mRenderPass = renderPass;
		mPipeline = generatePipeline(mDevice, mRenderPass, mPipelineInfo);
		setInitialized(true);
	}

	SkyboxGenerator::~SkyboxGenerator()
	{
		destroyMap(mDevice.device, mAllocator, mSkyboxMap);
	}

	void SkyboxGenerator::invalidate()
	{
		setInitialized(false);
		vkDestroyPipeline(mDevice.device, mPipeline, nullptr);
	}

	VkCommandBuffer& SkyboxGenerator::drawFrame(
		const VkCommandBufferInheritanceInfo& inheritance,
		const VkFramebuffer& framebuffer,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const Camera& camera)
	{
		// Update UBO
		UniformBufferObject ubo = {};
		ubo.model = camera.getWorldTransform();
		ubo.model[1][1] *= -1;
		ubo.view = camera.getViewTransform();
		ubo.modelViewProj = camera.getProjection() * ubo.view * ubo.model;
		ubo.cameraPos = camera.getWorldPosition();
		memcpy(mSkyboxRenderable.ubo.allocationInfo.pMappedData, &ubo, sizeof(ubo));

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

		vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mSkyboxRenderable.vbo.memoryResource, offsets);
		vkCmdBindIndexBuffer(mCommandBuffer, mSkyboxRenderable.ibo.memoryResource, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(
			mCommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			mPipelineInfo.pipelineLayout,
			0,
			1,
			&mDescriptorSet,
			0,
			nullptr);

		vkCmdDrawIndexed(mCommandBuffer, mSkyboxRenderable.numIndices, 1, 0, 0, 0);
		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}
}
