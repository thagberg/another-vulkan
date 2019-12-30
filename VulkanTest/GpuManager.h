#pragma once

#include "vk_mem_alloc.h"

#include "HvkUtil.h"

namespace hvk
{
    class GpuManager
    {
    private:
        static VkPhysicalDevice sPhysicalDevice;
        static VkDevice sDevice;
        static VkCommandPool sCommandPool;
        static VkQueue sGraphicsQueue;
        static VmaAllocator sAllocator;
        GpuManager();
        ~GpuManager();

    public:
        static void init(
            VkPhysicalDevice physicalDevice, 
            VkDevice device, 
            VkCommandPool commandPool, 
            VkQueue graphicsQueue, 
            VmaAllocator allocator);
        static const VkPhysicalDevice& getPhysicalDevice() { return sPhysicalDevice; }
        static const VkDevice& getDevice() { return sDevice; }
        static const VkCommandPool& getCommandPool() { return sCommandPool; }
        static const VkQueue& getGraphicsQueue() { return sGraphicsQueue; }
        static const VmaAllocator& getAllocator() { return sAllocator; }
    };
}
