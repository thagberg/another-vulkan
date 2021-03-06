#include "pch.h"
#include "image-util.h"

#include <assert.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"

#include "command-util.h"

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
				VkImageAspectFlags aspectFlags,
				uint32_t numLayers,
				VkImageViewType viewType,
				uint32_t mipLevels)
			{
				VkImageView imageView;

				VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
				createInfo.image = image;
				createInfo.viewType = viewType;
				createInfo.format = format;
				createInfo.subresourceRange.aspectMask = aspectFlags;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = mipLevels;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = numLayers;

				assert(vkCreateImageView(device, &createInfo, nullptr, &imageView) == VK_SUCCESS);

				return imageView;
			}


			VkSampler createImageSampler(
				VkDevice device,
				float maxLod)
			{
				VkSampler sampler;
				VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
				createInfo.magFilter = VK_FILTER_LINEAR;
				createInfo.minFilter = VK_FILTER_LINEAR;
				createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.anisotropyEnable = VK_TRUE;
				createInfo.maxAnisotropy = 1.f;
				createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				createInfo.mipLodBias = 0.f;
				createInfo.minLod = 0.f;
				createInfo.maxLod = maxLod;
				createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
				assert(vkCreateSampler(device, &createInfo, nullptr, &sampler) == VK_SUCCESS);

				return sampler;
			}


			void destroyMap(VkDevice device, VmaAllocator allocator, TextureMap& map)
			{
                vkDestroySampler(device, map.sampler, nullptr);
                vkDestroyImageView(device, map.view, nullptr);
                vmaDestroyImage(allocator, map.texture.memoryResource, map.texture.allocation);
			}


			hvk::RuntimeResource<VkImage> createTextureImage(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				const void* imageDataLayers,
				size_t numLayers,
				int imageWidth,
				int imageHeight,
				int bitDepth,
				VkImageType imageType,
				VkImageCreateFlags flags,
				VkFormat imageFormat) {

				hvk::RuntimeResource<VkImage> textureResource;

				//VkDeviceSize imageSize = imageWidth * imageHeight * components * bitDepth;
				VkDeviceSize singleImageSize = imageWidth * imageHeight * bitDepth;
				VkDeviceSize imageSize = singleImageSize * numLayers;

				// copy image data into a staging buffer which will then be used
				// to transfer to a Vulkan image
				VkBuffer imageStagingBuffer;
				VkBufferCreateInfo stagingCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				stagingCreateInfo.size = imageSize;
				stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VmaAllocation stagingAllocation;
				VmaAllocationInfo stagingAllocationInfo;
				VmaAllocationCreateInfo stagingAllocCreateInfo = {};
				stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

				vmaCreateBuffer(
					allocator,
					&stagingCreateInfo,
					&stagingAllocCreateInfo,
					&imageStagingBuffer,
					&stagingAllocation,
					&stagingAllocationInfo);

				void* stagingData;
				int offset = 0;
				vmaMapMemory(allocator, stagingAllocation, &stagingData);
				memcpy(stagingData, imageDataLayers, imageSize);
				vmaUnmapMemory(allocator, stagingAllocation);

				VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
				imageInfo.imageType = imageType;
				imageInfo.extent.width = static_cast<uint32_t>(imageWidth);
				imageInfo.extent.height = static_cast<uint32_t>(imageHeight);
				imageInfo.extent.depth = 1;
				imageInfo.mipLevels = 1;
				imageInfo.arrayLayers = numLayers;
				imageInfo.format = imageFormat;
				imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageInfo.flags = flags;

				VmaAllocationCreateInfo imageAllocationCreateInfo = {};
				imageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

				auto createResult = vmaCreateImage(
					allocator,
					&imageInfo,
					&imageAllocationCreateInfo,
					&textureResource.memoryResource,
					&textureResource.allocation,
                    nullptr);

				auto commandBuffer = command::beginSingleTimeCommand(device, commandPool);
				transitionImageLayout(
					commandBuffer,
					textureResource.memoryResource,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					numLayers);
				command::endSingleTimeCommand(device, commandPool, commandBuffer, graphicsQueue);

				copyBufferToImage(
					device,
					commandPool,
					graphicsQueue,
					imageStagingBuffer,
					textureResource.memoryResource,
					imageWidth,
					imageHeight,
					numLayers,
					singleImageSize);

				commandBuffer = command::beginSingleTimeCommand(device, commandPool);
				transitionImageLayout(
					commandBuffer,
					textureResource.memoryResource,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					//1);
					numLayers);
				command::endSingleTimeCommand(device, commandPool, commandBuffer, graphicsQueue);


				vmaDestroyBuffer(allocator, imageStagingBuffer, stagingAllocation);

				return textureResource;
			}

			TextureMap createTextureMap(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				const void* imageData,
				int imageWidth,
				int imageHeight,
				int bitDepth,
				VkImageType imageType,
				VkImageCreateFlags flags,
				VkFormat imageFormat)
			{
				TextureMap map;

				map.texture = createTextureImage(
					device,
					allocator,
					commandPool,
					graphicsQueue,
					imageData,
					1,
					imageWidth,
					imageHeight,
					bitDepth);
				map.view = createImageView(
					device,
					map.texture.memoryResource,
					VK_FORMAT_R8G8B8A8_UNORM,
					VK_IMAGE_ASPECT_COLOR_BIT,
					1);
				map.sampler = createImageSampler(device);

				return map;
			}


			TextureMap createTextureMapFromFile(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				std::string&& filename)
			{
				TextureMap map;

				int width, height, numChannels;
				unsigned char* data = stbi_load(filename.c_str(), &width, &height, &numChannels, 0);

                assert(data != nullptr);

				map = createTextureMap(
					device,
					allocator,
					commandPool,
					graphicsQueue,
					data,
					width,
					height,
					numChannels);

				stbi_image_free(data);
				return map;
			}


			TextureMap createCubeMap(
				VkDevice device, 
				VmaAllocator allocator, 
				VkCommandPool commandPool, 
				VkQueue graphicsQueue, 
				std::array<std::string, 6>& fileNames)
			{
				TextureMap cubeMap;

				// load cubemap textures
				/*
				
						T
					|L |F |R |BA
						BO
				
				*/
				int width, height, numChannels;
				int copyOffset = 0;
				int copySize = 0;
				int layerSize = 0;
				unsigned char* copyTo = nullptr;
				unsigned char* layers[6];
				for (size_t i = 0; i < fileNames.size(); ++i)
				{
					unsigned char* data = stbi_load(
						fileNames[i].c_str(), 
						&width, 
						&height, 
						&numChannels, 
						0);

					assert(data != nullptr);

					layers[i] = data;
					copySize += width * height * numChannels;
				}
				layerSize = copySize / fileNames.size();

                copyTo = static_cast<unsigned char*>(_aligned_malloc(copySize, alignof(unsigned char)));
				for (size_t i = 0; i < 6; ++i)
				{
					void* dst = copyTo + copyOffset;
					copyOffset += layerSize;
					memcpy(dst, layers[i], layerSize);
					stbi_image_free(layers[i]);
				}

				cubeMap.texture = createTextureImage(
					device,
					allocator,
					commandPool,
					graphicsQueue,
					copyTo,
					6,
					width,
					height,
					numChannels,
					VK_IMAGE_TYPE_2D,
					VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
				cubeMap.view = createImageView(
					device,
					cubeMap.texture.memoryResource,
					VK_FORMAT_R8G8B8A8_UNORM,
					VK_IMAGE_ASPECT_COLOR_BIT,
					1,
					VK_IMAGE_VIEW_TYPE_CUBE);
				cubeMap.sampler = createImageSampler(device);

                free(copyTo);

				return cubeMap;
			}

			void copyBufferToImage(
				VkDevice device,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				VkBuffer buffer,
				VkImage image,
				uint32_t width,
				uint32_t height,
				size_t numFaces,
				size_t faceSize) {

				VkCommandBuffer commandBuffer = command::beginSingleTimeCommand(device, commandPool);


				std::vector<VkBufferImageCopy> faceRegions;
				faceRegions.reserve(numFaces);

				uint32_t offset = 0;
				for (size_t i = 0; i < numFaces; ++i)
				{
					VkBufferImageCopy region = {};
					region.bufferOffset = offset;
					region.bufferRowLength = 0;
					region.bufferImageHeight = 0;
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.mipLevel = 0;
					region.imageSubresource.baseArrayLayer = i;
					region.imageSubresource.layerCount = 1;
					region.imageOffset = { 0, 0, 0 };
					region.imageExtent = { width, height, 1 };

					offset += faceSize;
					faceRegions.push_back(region);
				}

				vkCmdCopyBufferToImage(
					commandBuffer, 
					buffer, 
					image, 
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
					numFaces, 
					faceRegions.data());

				command::endSingleTimeCommand(device, commandPool, commandBuffer, graphicsQueue);
			}


			TextureMap createImageMap(
				VkDevice device,
				VmaAllocator allocator,
				VkCommandPool commandPool,
				VkQueue graphicsQueue,
				VkFormat imageFormat,
				uint32_t imageWidth,
				uint32_t imageHeight,
				VkImageCreateFlags createFlags,
				VkImageUsageFlags usageFlags,
				uint32_t arrayLayers,
				VkImageLayout initialLayout,
				VkImageViewType viewType,
				uint32_t mipLevels,
				VkImageAspectFlags aspectFlags)
			{
				TextureMap imageMap;
				uint32_t bitDepth = 4;
				if (imageFormat == VK_FORMAT_R8G8B8_UNORM) {
					bitDepth = 3;
				}

				VkImageCreateInfo image = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
				image.imageType = VK_IMAGE_TYPE_2D;
				image.format = imageFormat;
				image.extent.width = imageWidth;
				image.extent.height = imageHeight;
				image.extent.depth = 1;
				image.mipLevels = mipLevels;
				image.arrayLayers = arrayLayers;
				image.samples = VK_SAMPLE_COUNT_1_BIT;
				image.tiling = VK_IMAGE_TILING_OPTIMAL;
				image.initialLayout = initialLayout;
				image.usage = usageFlags;
				image.flags = createFlags;

				VmaAllocationCreateInfo imageAllocationCreateInfo = {};
				imageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

				vmaCreateImage(
					allocator,
					&image,
					&imageAllocationCreateInfo,
					&imageMap.texture.memoryResource,
					&imageMap.texture.allocation,
                    nullptr);

				imageMap.view = createImageView(
					device,
					imageMap.texture.memoryResource,
					imageFormat,
					aspectFlags,
					arrayLayers,
					viewType,
					mipLevels);

				imageMap.sampler = createImageSampler(device, static_cast<float>(mipLevels-1));

				return imageMap;
			}


			void transitionImageLayout(
				VkCommandBuffer commandBuffer,
				VkImage image,
				VkImageLayout oldLayout,
				VkImageLayout newLayout,
				uint32_t numLayers /* 1 */,
				uint32_t baseLayer /* 0 */,
				uint32_t mipLevels /* 1 */,
				uint32_t baseMipLevel /* 0 */,
				VkImageAspectFlags aspectMask /* VK_IMAGE_ASPECT_COLOR_BIT */) {

				VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;

				barrier.subresourceRange.aspectMask = aspectMask;
				if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					// TODO: potentially |= VK_IMAGE_ASPECT_STENCIL_BIT if the format supports stencil
				} 
				//else {
				//	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				//}

				barrier.subresourceRange.baseMipLevel = baseMipLevel;
				barrier.subresourceRange.levelCount = mipLevels;
				barrier.subresourceRange.baseArrayLayer = baseLayer;
				barrier.subresourceRange.layerCount = numLayers;

				VkPipelineStageFlags sourceStage;
				VkPipelineStageFlags destinationStage;

				switch (oldLayout)
				{
				case VK_IMAGE_LAYOUT_UNDEFINED:
					barrier.srcAccessMask = 0;
					break;
				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;
				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;
				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				default:
					break;
				}

				switch (newLayout)
				{
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;
				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;
				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;
				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					if (barrier.srcAccessMask == 0)
					{
						barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
					}
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				default:
					break;
				}

				// TODO: parameterize this
				sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

				vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			}

			void framebufferImageToTexture(
				VkCommandBuffer commandBuffer,
				const TextureMap& framebufferImage,
				TextureMap& copyMap)
			{
				//auto copiedMap = createImageMap(
				//	device,
				//	allocator,
				//	commandPool,
				//	graphicsQueue,
				//	VK_FORMAT_D32_SFLOAT,
				//	2048,
				//	2048,
				//	0,
				//	VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

				//auto onetime = command::beginSingleTimeCommand(device, commandPool);
				//transitionImageLayout(
				//	onetime,
				//	copyMap.texture.memoryResource,
				//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				//command::endSingleTimeCommand(device, commandPool, onetime, graphicsQueue);

				// copy framebuffer to the new texture
				VkImageCopy copyRegion = {};
				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				copyRegion.dstSubresource.baseArrayLayer = 0;
				copyRegion.dstSubresource.mipLevel = 0;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = 2048;
				copyRegion.extent.height = 2048;
				copyRegion.extent.depth = 1;

				//auto onetime = command::beginSingleTimeCommand(device, commandPool);
				vkCmdCopyImage(
					commandBuffer,
					framebufferImage.texture.memoryResource,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					copyMap.texture.memoryResource,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion);
				//command::endSingleTimeCommand(device, commandPool, onetime, graphicsQueue);

				// transition image for shader read
				//onetime = command::beginSingleTimeCommand(device, commandPool);
				transitionImageLayout(
					commandBuffer,
					copyMap.texture.memoryResource,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					1,
					0,
					1,
					0,
					VK_IMAGE_ASPECT_DEPTH_BIT);
				//command::endSingleTimeCommand(device, commandPool, onetime, graphicsQueue);
			}
		}
	}
}