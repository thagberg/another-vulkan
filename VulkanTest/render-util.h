#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif
#include "vk_mem_alloc.h"

#include "types.h"

namespace hvk
{
	namespace util
	{
		namespace render
		{
			VkFence renderCubeMap(
				VulkanDevice& device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				VkCommandBuffer commandBuffer,
				HVK_shared<TextureMap> inMap,
				uint32_t outResolution,
				VkFormat outFormat,
				HVK_shared<TextureMap> outMap,
				std::array<std::string, 2>& shaderFiles,
				const GammaSettings& gammaSettings);
		}
	}
}
