#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "types.h"

namespace hvk
{
	namespace util
	{
		namespace image
		{
			VkImageView createImageView(
				VkDevice device, 
				VkImage image, 
				VkFormat format, 
				VkImageAspectFlags aspectFlags=VK_IMAGE_ASPECT_COLOR_BIT,
				uint32_t numLayers=1,
				VkImageViewType viewType=VK_IMAGE_VIEW_TYPE_2D,
				uint32_t mipLevels=1);

			VkSampler createImageSampler(
				VkDevice device,
				float maxLod=0.f);

			void transitionImageLayout(
				VkCommandBuffer commandBuffer,
				VkImage image,
				VkImageLayout oldLayout,
				VkImageLayout newLayout,
				uint32_t numLayers=1,
				uint32_t baseLayer=0,
				uint32_t mipLevels=1,
				uint32_t baseMipLevel=0);

			TextureMap createImageMap(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				VkFormat imageFormat,
				uint32_t imageWidth,
				uint32_t imageHeight,
				VkImageCreateFlags createFlags = 0,
				VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				uint32_t arrayLayers = 1,
				VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				VkImageViewType viewType=VK_IMAGE_VIEW_TYPE_2D,
				uint32_t mipLevels = 1);

			Resource<VkImage> createTextureImage(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				const void* imageDataLayers,
				size_t numLayers,
				int imageWidth,
				int imageHeight,
				int bitDepth,
				VkImageType imageType=VK_IMAGE_TYPE_2D,
				VkImageCreateFlags flags=0,
				VkFormat imageFormat=VK_FORMAT_R8G8B8A8_UNORM);

			TextureMap createCubeMap(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				std::array<std::string, 6>& fileNames);

			TextureMap createTextureMap(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				std::string&& filename);

			// TODO: Maybe move to a memory util instead?
			void destroyMap(VkDevice device, VmaAllocator allocator, TextureMap& map);

			void copyBufferToImage(
				VkDevice device,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				VkBuffer buffer,
				VkImage image,
				uint32_t width,
				uint32_t height,
				size_t numFaces = 1,
				size_t faceSize = 0);
		}
	}
}
