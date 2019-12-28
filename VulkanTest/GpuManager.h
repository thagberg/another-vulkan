#pragma once

#include "vk_mem_alloc.h"

#include "HvkUtil.h"

namespace hvk
{
    class GpuManager
    {
    private:
        static VkDevice sDevice;
        static VkCommandPool sCommandPool;
        static VkQueue sGraphicsQueue;
        static VmaAllocator sAllocator;
        GpuManager();
        ~GpuManager();

    public:
        static void init(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VmaAllocator allocator);
        static const VkDevice& getDevice() { return sDevice; }
        static const VkCommandPool& getCommandPool() { return sCommandPool; }
        static const VkQueue& getGraphicsQueue() { return sGraphicsQueue; }
        static const VmaAllocator& getAllocator() { return sAllocator; }
    };
}
