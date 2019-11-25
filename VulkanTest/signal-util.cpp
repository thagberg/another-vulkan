#include "pch.h"
#include "signal-util.h"

#include <assert.h>

namespace hvk
{
	namespace util
	{
		namespace signal
		{
			VkSemaphore createSemaphore(VkDevice device) {
				VkSemaphore semaphore;

				VkSemaphoreCreateInfo semaphoreInfo = {};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				assert(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) == VK_SUCCESS);

				return semaphore;
			}
		}
	}
}