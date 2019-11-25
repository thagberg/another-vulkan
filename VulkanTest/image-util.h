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
				VkImageViewType viewType=VK_IMAGE_VIEW_TYPE_2D);

			VkSampler createImageSampler(
				VkDevice device);

			void transitionImageLayout(
				VkDevice device,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				VkImage image,
				VkFormat format,
				VkImageLayout oldLayout,
				VkImageLayout newLayout,
				uint32_t numLayers=1);

			TextureMap createImageMap(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				VkFormat imageFormat,
				uint32_t imageWidth,
				uint32_t imageHeight,
				VkImageUsageFlags flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

			hvk::Resource<VkImage> createTextureImage(
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
