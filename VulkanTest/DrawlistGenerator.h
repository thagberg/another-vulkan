#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "types.h"
#include "Node.h"
#include "Camera.h"
#include "Renderer.h"

namespace hvk
{

    class DrawlistGenerator
    {
    private:
        bool mInitialized;

        VkRenderPass mRenderPass;
        VulkanDevice mDevice;
        VkQueue mGraphicsQueue;
        VmaAllocator mAllocator;

        VkViewport mViewport;
        VkRect2D mScissor;

        VkFence mRenderFence;
        VkSemaphore mRenderFinished;
        VkCommandPool mCommandPool;
        VkCommandBuffer mCommandBuffer;
        VkDescriptorPool mDescriptorPool;
        VkDescriptorSetLayout mDescriptorSetLayout;

        DrawlistGenerator();
        void setInitialized(bool init) { mInitialized = init; }

    public:
        virtual ~DrawlistGenerator();

        bool getInitialized() const { return mInitialized; }
        virtual VkSemaphore drawFrame(const VkFramebuffer& framebuffer, const VkSemaphore* waitSemaphores = nullptr, uint32_t waitSemaphoreCount = 0) = 0;
    };
}
