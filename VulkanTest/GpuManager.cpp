#include "pch.h"
#include "GpuManager.h"


namespace hvk
{
    VkDevice GpuManager::sDevice = VK_NULL_HANDLE;
    VkCommandPool GpuManager::sCommandPool = VK_NULL_HANDLE;
    VkQueue GpuManager::sGraphicsQueue = VK_NULL_HANDLE;
    VmaAllocator GpuManager::sAllocator = VK_NULL_HANDLE;

    GpuManager::GpuManager()
    {
    }


    GpuManager::~GpuManager()
    {
    }

    void GpuManager::init(
        VkPhysicalDevice physicalDevice, 
        VkDevice device, 
        VkCommandPool commandPool, 
        VkQueue graphicsQueue, 
        VmaAllocator allocator)
    {
        sPhysicalDevice = physicalDevice;
        sDevice = device;
        sCommandPool = commandPool;
        sGraphicsQueue = graphicsQueue;
        sAllocator = allocator;
    }
}
