#include "UiDrawGenerator.h"
//#define HVK_UTIL_IMPLEMENTATION
#include "vulkan-util.h"

#include "imgui/imgui.h"

namespace hvk
{
    void setIOSizes(ImGuiIO& io, const VkExtent2D& displaySize, const ImVec2& fbScale)
    {
        io.DisplaySize = ImVec2(
            static_cast<float>(displaySize.width), 
            static_cast<float>(displaySize.height));
        io.DisplayFramebufferScale = fbScale;
    }

	UiDrawGenerator::UiDrawGenerator(
		VulkanDevice device,
		VmaAllocator allocator,
		VkQueue graphicsQueue,
		VkRenderPass renderPass,
		VkCommandPool commandPool,
		VkExtent2D windowExtent) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass, commandPool),
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
        setIOSizes(io, mWindowExtent, ImVec2(1.f, 1.f));

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
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
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
        setInitialized(false);
		vkDestroyPipeline(mDevice.device, mPipeline, nullptr);
	}

	void UiDrawGenerator::updateRenderPass(VkRenderPass renderPass, VkExtent2D windowExtent)
	{
		mWindowExtent = windowExtent;
		mRenderPass = renderPass;
		mPipeline = generatePipeline(mDevice, mRenderPass, mPipelineInfo);

		ImGuiIO& io = ImGui::GetIO();
        setIOSizes(io, mWindowExtent, ImVec2(1.f, 1.f));

		setInitialized(true);
	}

	VkCommandBuffer& UiDrawGenerator::drawFrame(
        const VkCommandBufferInheritanceInfo& inheritance,
		VkFramebuffer& framebuffer,
		const VkViewport& viewport,
		const VkRect2D& scissor)
	{
		ImGuiIO& io = ImGui::GetIO();
		//ImGui::NewFrame();
		//ImGui::ShowDemoWindow();
		//ImGui::EndFrame();
		ImGui::Render();

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBegin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBegin.pInheritanceInfo = &inheritance;

		assert(vkBeginCommandBuffer(mCommandBuffer, &commandBegin) == VK_SUCCESS);
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

		assert(vkEndCommandBuffer(mCommandBuffer) == VK_SUCCESS);

		return mCommandBuffer;
	}
}