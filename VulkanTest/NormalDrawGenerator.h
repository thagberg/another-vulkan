#pragma once
#include "DrawlistGenerator.h"
#include "types.h"

namespace hvk
{
	class NormalDrawGenerator : public DrawlistGenerator
	{
	private:
		VkDescriptorSetLayout mDescriptorSetLayout;
		VkDescriptorPool mDescriptorPool;
		VkPipeline mPipeline;
		RenderPipelineInfo mPipelineInfo;

		void preparePipelineInfo();

	public:
		NormalDrawGenerator(VkRenderPass renderPass, VkCommandPool commandPool);
		virtual ~NormalDrawGenerator();
		virtual void invalidate() override;
		void updateRenderPass(VkRenderPass renderPass);
	};
}

