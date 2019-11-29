#include "pch.h"
#include "render-util.h"

#include "signal-util.h"
#include "image-util.h"
#include "renderpass-util.h"
#include "framebuffer-util.h"
#include "CubemapGenerator.h"
#include "command-util.h"
#include "camera.h"

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
				const GammaSettings& gammaSettings,
				uint32_t mipLevels /* 1 */)
			{
				auto cubeMap = HVK_make_shared<TextureMap>(image::createImageMap(
					device.device,
					allocator,
					commandPool,
					graphicsQueue,
					outFormat,
					outResolution,
					outResolution,
					0,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
				auto cubeColorAttachment = renderpass::createColorAttachment(
					outFormat,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				std::vector<VkSubpassDependency> cubeColorPassDependencies = {
					renderpass::createSubpassDependency()
				};
				auto cubeRenderPass = renderpass::createRenderPass(
					device.device,
					outFormat,
					cubeColorPassDependencies, 
					&cubeColorAttachment);
				VkExtent2D cubeExtent = {
					outResolution,
					outResolution,
				};
				VkFramebuffer cubeFrameBuffer;
				framebuffer::createFramebuffer(
					device.device,
					cubeRenderPass,
					cubeExtent,
					cubeMap->view, 
					nullptr, 
					&cubeFrameBuffer);

				CubemapGenerator cubeRenderer(
					device,
					allocator,
					graphicsQueue,
					cubeRenderPass,
					commandPool,
					inMap,
					shaderFiles);

				// Create cubemap which we will iteratively copy environmentFramebuffer onto
				*outMap = image::createImageMap(
					device.device,
					allocator,
					commandPool,
					graphicsQueue,
					outFormat,
					outResolution,
					outResolution,
					VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
					VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					6,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_VIEW_TYPE_CUBE);

				// map needs to be transitioned to a transfer destination
				auto onetime = command::beginSingleTimeCommand(device.device, commandPool);
				util::image::transitionImageLayout(
					onetime,
					outMap->texture.memoryResource,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					6,
					0);
				command::endSingleTimeCommand(device.device, commandPool, onetime, graphicsQueue);

				// Start getting ready for the renders
				VkFence renderFence = signal::createFence(device.device);

				VkViewport viewport = {};
				viewport.x = 0.f;
				viewport.y = 0.f;
				viewport.width = static_cast<float>(outResolution);
				viewport.height = static_cast<float>(outResolution);
				viewport.minDepth = 0.f;
				viewport.maxDepth = 1.f;
				VkRect2D scissor = {};
				scissor.offset = { 0, 0 };
				scissor.extent = cubeExtent;

				std::array<VkClearValue, 1> clearValues = {};
				clearValues[0].color = { 0.f, 0.f, 0.f, 1.0f };

				VkCommandBufferBeginInfo commandBegin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				commandBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				commandBegin.pInheritanceInfo = nullptr;

				VkRenderPassBeginInfo renderBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
				renderBegin.renderPass = cubeRenderPass;
				renderBegin.framebuffer = cubeFrameBuffer;
				renderBegin.renderArea = scissor;
				renderBegin.clearValueCount = static_cast<float>(clearValues.size());
				renderBegin.pClearValues = clearValues.data();

				VkCommandBufferInheritanceInfo inheritanceInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
				inheritanceInfo.pNext = nullptr;
				inheritanceInfo.renderPass = cubeRenderPass;
				inheritanceInfo.subpass = 0;
				inheritanceInfo.framebuffer = cubeFrameBuffer;
				inheritanceInfo.occlusionQueryEnable = VK_FALSE;

				// Create a camera which we will use to face each wall of the cube which we are rendering to
				// For each face, we sample the HDR texture in spherical coordinates and map that to the wall
				// of the cube, which can then be copied from the framebuffer's color attachment and into
				// an image that we can treat as a texture
				auto cubeCamera = Camera(90.f, 1.f, 0.01f, 1000.f, std::string("CubeCamera"), nullptr, glm::mat4(1.f));
				std::array<glm::mat4, 6> cameraTransforms = {
					glm::lookAt(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)),
					glm::lookAt(glm::vec3(0.f), glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)),
					glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, 1.f)),
					glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, -1.f)),
					glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f)),
					glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f))
				};

				// iterate over each face of the cube, point a camera towards that, render it, and
				// copy the framebuffer's color attachment to the new cubemap
				for (uint8_t i = 0; i < 6; ++i)
				{
					// prepare camera
					const auto& cameraTransform = cameraTransforms[i];
					cubeCamera.setLocalTransform(cameraTransform);

					for (uint32_t j = 0; j < mipLevels; ++j)
					{
						vkBeginCommandBuffer(commandBuffer, &commandBegin);
						vkCmdBeginRenderPass(commandBuffer, &renderBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

						auto cubemapCommandBuffer = cubeRenderer.drawFrame(
							inheritanceInfo, 
							cubeFrameBuffer, 
							viewport, 
							scissor, 
							cubeCamera,
							gammaSettings);

						vkCmdExecuteCommands(commandBuffer, 1, &cubemapCommandBuffer);
						vkCmdEndRenderPass(commandBuffer);

						// prepare to copy framebuffer over to cubemap
						image::transitionImageLayout(
							commandBuffer,
							cubeMap->texture.memoryResource,
							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
							VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							1);

						// now copy it over
						VkImageCopy copyRegion = {};

						copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copyRegion.srcSubresource.baseArrayLayer = 0;
						copyRegion.srcSubresource.mipLevel = 0;
						copyRegion.srcSubresource.layerCount = 1;
						copyRegion.srcOffset = { 0, 0, 0 };

						copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copyRegion.dstSubresource.baseArrayLayer = i;
						copyRegion.dstSubresource.mipLevel = j;
						copyRegion.dstSubresource.layerCount = 1;
						copyRegion.dstOffset = { 0, 0, 0 };

						copyRegion.extent.width = outResolution;
						copyRegion.extent.height = outResolution;
						copyRegion.extent.depth = 1;

						vkCmdCopyImage(
							commandBuffer,
							cubeMap->texture.memoryResource,
							VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							outMap->texture.memoryResource,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							1,
							&copyRegion);

						// transition framebuffer back to color attachment
						image::transitionImageLayout(
							commandBuffer,
							cubeMap->texture.memoryResource,
							VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

						// transition outMap face to shader read
						image::transitionImageLayout(
							commandBuffer,
							outMap->texture.memoryResource,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
							1,
							i,
							j);

						vkEndCommandBuffer(commandBuffer);

						VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
						VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
						submitInfo.waitSemaphoreCount = 0;
						submitInfo.pWaitSemaphores = nullptr;
						submitInfo.pWaitDstStageMask = waitStages;
						submitInfo.commandBufferCount = 1;
						submitInfo.pCommandBuffers = &commandBuffer;
						submitInfo.signalSemaphoreCount = 0;
						submitInfo.pSignalSemaphores = nullptr;

						assert(vkResetFences(device.device, 1, &renderFence) == VK_SUCCESS);
						assert(vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFence) == VK_SUCCESS);
						assert(vkWaitForFences(device.device, 1, &renderFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);
					}
				}

				return renderFence;
			}
		}
	}
}