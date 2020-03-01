#include "NormalDrawGenerator.h"
#include "descriptor-util.h"
#include "pipeline-util.h"

hvk::NormalDrawGenerator::NormalDrawGenerator(VkRenderPass renderPass, VkCommandPool commandPool) :
	DrawlistGenerator(renderPass, commandPool),
	mDescriptorSetLayout(VK_NULL_HANDLE),
	mDescriptorPool(VK_NULL_HANDLE),
	mPipeline(VK_NULL_HANDLE),
	mPipelineInfo()
{
	const VkDevice& device = GpuManager::getDevice();
	const VmaAllocator& allocator = GpuManager::getAllocator();

	// Create descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> bindings = {
		util::descriptor::generateUboLayoutBinding(0, 1),		// UBO binding
		util::descriptor::generateSamplerLayoutBinding(1, 1)	// Normal map sampler binding
	};
	util::descriptor::createDescriptorSetLayout(device, bindings, mDescriptorSetLayout);

	// Create descriptor pool
	auto poolSizes = util::descriptor::createPoolSizes<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(MAX_UBOS, MAX_SAMPLERS);

	// Prepare pipeline
	preparePipelineInfo();

	setInitialized(true);
}

hvk::NormalDrawGenerator::~NormalDrawGenerator()
{
}

void hvk::NormalDrawGenerator::invalidate()
{
	setInitialized(false);
	vkDestroyPipeline(GpuManager::getDevice(), mPipeline, nullptr);
}

void hvk::NormalDrawGenerator::updateRenderPass(VkRenderPass renderPass)
{
	mColorRenderPass = renderPass;
	mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
	setInitialized(true);
}

void hvk::NormalDrawGenerator::preparePipelineInfo()
{
	const auto& device = GpuManager::getDevice();

	VkPipelineLayoutCreateInfo layoutCreate = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreate.setLayoutCount = 1;
	layoutCreate.pSetLayouts = &mDescriptorSetLayout;
	layoutCreate.pushConstantRangeCount = 0;

	assert(vkCreatePipelineLayout(device, &layoutCreate, nullptr, &mPipelineInfo.pipelineLayout) == VK_SUCCESS);

	util::pipeline::fillVertexInfo<Vertex>(mPipelineInfo.vertexInfo);

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable = VK_FALSE;
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM;

	mPipelineInfo.blendAttachments = { blendAttachment };
	mPipelineInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	mPipelineInfo.vertShaderFile = "shaders/compiled/normal_v.spv";
	mPipelineInfo.fragShaderFile = "shaders/compiled/normal_f.spv";
	mPipelineInfo.depthStencilState = util::pipeline::createDepthStencilState();
	mPipelineInfo.rasterizationState = util::pipeline::createRasterizationState();

	mPipeline = generatePipeline(mColorRenderPass, mPipelineInfo);
}

