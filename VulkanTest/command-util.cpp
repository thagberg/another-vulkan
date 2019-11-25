#include "pch.h"
#include "command-util.h"

#include <assert.h>

namespace hvk
{
	namespace util
	{
		namespace command
		{
			VkCommandBuffer beginSingleTimeCommand(VkDevice device, VkCommandPool commandPool) {
				VkCommandBuffer commandBuffer;

				VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandPool = commandPool;
				allocInfo.commandBufferCount = 1;

				vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

				VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(commandBuffer, &beginInfo);
				return commandBuffer;
			}


			void endSingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue) {
				vkEndCommandBuffer(commandBuffer);

				VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(queue);
				vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
			}


			VkCommandPool createCommandPool(
				VkDevice device, 
				int queueFamilyIndex, 
				VkCommandPoolCreateFlags flags) {

				VkCommandPool commandPool;

				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = queueFamilyIndex;
				poolInfo.flags = flags;

				assert(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS);

				return commandPool;
			}
		}
	}
}