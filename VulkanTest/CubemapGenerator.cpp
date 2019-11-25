#include "CubemapGenerator.h"

#include "stb_image.h"

#include "vulkan-util.h"
#include "ResourceManager.h"
#include "shapes.h"


namespace hvk
{
	CubemapGenerator:: CubemapGenerator(
		VulkanDevice device, 
		VmaAllocator allocator, 
		VkQueue graphicsQueue, 
		VkRenderPass renderPass, 
		VkCommandPool commandPool,
        HVK_shared<TextureMap> skyboxMap,
		std::array<std::string, 2>& shaderFiles) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass, commandPool),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorPool(VK_NULL_HANDLE),
		mDescriptorSet(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineInfo(),
		mCubeMap(skyboxMap),
		mCubeRenderable()
	{
		//auto cube = createColoredCube(glm::vec3(0.f, 1.f, 0.f));
        auto cube = createEnvironmentCube();
		mCubeRenderable.renderObject = HVK_make_shared<CubeMeshRenderObject>(
			"Sky Box",
			nullptr,
			glm::mat4(1.f),
			cube);

		mCubeRenderable.sRGB = true;

		auto skyboxMesh = createColoredCube(glm::vec3(0.1f, 4.f, 1.f));
		auto vertices = skyboxMesh->getVertices();
		auto indices = skyboxMesh->getIndices();
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
			mAllocator,
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
            mAllocator,
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
			mAllocator,
			&uboInfo,
			&uniformAllocCreateInfo,
			&mCubeRenderable.ubo.memoryResource,
			&mCubeRenderable.ubo.allocation,
			&mCubeRenderable.ubo.allocationInfo);

		// Create descriptor pool
		auto poolSizes = createPoolSizes<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(1, 1);
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
		auto bufferWrite = createDescriptorBufferWrite(bufferInfos, mDescriptorSet, 0);

		std::vector<VkDescriptorImageInfo> imageInfos = { imageInfo };
		auto imageWrite = createDescriptorImageWrite(imageInfos, mDescriptorSet, 1);

		std::vector<VkWriteDescriptorSet> descriptorWrites = {
			bufferWrite,
            imageWrite
		};
		writeDescriptorSets(mDevice.device, descriptorWrites);


		// Prepare pipeline
		VkPushConstantRange push = {};
		push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		push.offset = 0;
		push.size = sizeof(PushConstant);

		VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreate.setLayoutCount = static_cast<uint32_t>(layouts.size());
		layoutCreate.pSetLayouts = layouts.data();
		layoutCreate.pushConstantRangeCount = 1;
		layoutCreate.pPushConstantRanges = &push;
		assert(vkCreatePipelineLayout(mDevice.device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

		fillVertexInfo<hvk::CubeVertex>(mPipelineInfo.vertexInfo);

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		mPipelineInfo.blendAttachments = { blendAttachment };
		mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mPipelineInfo.vertShaderFile = shaderFiles[0].c_str();
		mPipelineInfo.fragShaderFile = shaderFiles[1].c_str();
		mPipelineInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		mPipelineInfo.depthStencilState = createDepthStencilState(true, true);

		mPipeline = generatePipeline(mDevice, mColorRenderPass, mPipelineInfo);
		

		setInitialized(true);
	}

	void CubemapGenerator::updateRenderPass(VkRenderPass renderPass)
	{
		mColorRenderPass = renderPass;
		mPipeline = generatePipeline(mDevice, mColorRenderPass, mPipelineInfo);
		setInitialized(true);
	}

	CubemapGenerator::~CubemapGenerator()
	{
		vmaDestroyBuffer(mAllocator, mCubeRenderable.vbo.memoryResource, mCubeRenderable.vbo.allocation);
		vmaDestroyBuffer(mAllocator, mCubeRenderable.ibo.memoryResource, mCubeRenderable.ibo.allocation);
		vmaDestroyBuffer(mAllocator, mCubeRenderable.ubo.memoryResource, mCubeRenderable.ubo.allocation);
		vkDestroyDescriptorSetLayout(mDevice.device, mDescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(mDevice.device, mDescriptorPool, nullptr);
		vkDestroyPipeline(mDevice.device, mPipeline, nullptr);
		vkDestroyPipelineLayout(mDevice.device, mPipelineInfo.pipelineLayout, nullptr);
		mCubeMap.reset();
	}

	void CubemapGenerator::invalidate()
	{
		setInitialized(false);
		vkDestroyPipeline(mDevice.device, mPipeline, nullptr);
	}

	VkCommandBuffer& CubemapGenerator::drawFrame(
		const VkCommandBufferInheritanceInfo& inheritance,
		const VkFramebuffer& framebuffer,
		const VkViewport& viewport,
		const VkRect2D& scissor,
		const Camera& camera,
		const GammaSettings& gammaSettings)
	{
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

		PushConstant push = { gammaSettings.gamma, mCubeRenderable.sRGB };
		vkCmdPushConstants(
			mCommandBuffer, 
			mPipelineInfo.pipelineLayout, 
			VK_SHADER_STAGE_FRAGMENT_BIT, 
			0, 
			sizeof(PushConstant), 
			&push);

		vkCmdDrawIndexed(mCommandBuffer, mCubeRenderable.numIndices, 1, 0, 0, 0);
		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}
}
