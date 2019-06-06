// VulkanTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <vector>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>


const int HEIGHT = 600;
const int WIDTH = 800;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Swapchain {
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
};

GLFWwindow* initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test Window", nullptr, nullptr);
    return window;
}

void cleanup(GLFWwindow* window, VkSurfaceKHR surface, VkDevice device, VkInstance instance) {
    glfwDestroyWindow(window);
    glfwTerminate();
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back("VK_EXT_debug_utils");

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* pDebugMesenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, nullptr, pDebugMesenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& presentMode : availablePresentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        } else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = presentMode;
        }
    }

    return bestMode;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D extent = { WIDTH, HEIGHT };

        extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
        extent.width = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));

        return extent;
    }
}

VkResult createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, Swapchain& swapchain) {
    SwapchainSupportDetails support = querySwapchainSupport(physicalDevice, surface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
    VkExtent2D extent = chooseSwapExtent(support.capabilities);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount && support.capabilities.maxImageCount < imageCount) {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // relying on graphics and present queues being the same
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = VK_NULL_HANDLE;
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    swapchain.swapchainImageFormat = surfaceFormat.format;
    swapchain.swapchainExtent = extent;

    return vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain.swapchain);
}

int main()
{
    GLFWwindow* window = initWindow();

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " extensions supported." << std::endl;

    // initialize Vulkan
    VkInstance instance;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    std::vector<const char*> extensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &instance);

    if (instanceResult != VK_SUCCESS) {
        return -1;
    }

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = debugCallback;
    debugInfo.pUserData = nullptr;

    VkDebugUtilsMessengerEXT debugMessenger;
    VkResult debugResult = CreateDebugUtilsMessengerEXT(instance, &debugInfo, &debugMessenger);

    if (debugResult != VK_SUCCESS) {
        return -2;
    }

    // Cheating... just take the first physical device found
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures deviceFeatures = {};
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physicalDevice = devices[0];

    uint32_t queueFamilyCount = 0;
    uint32_t graphicsIndex = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDevice device;
    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    //deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //deviceInfo.ppEnabledLayerNames = validationLayers.data();

    VkResult deviceStatus = vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);
    if (deviceStatus != VK_SUCCESS) {
        return -3;
    }

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, graphicsIndex, 0, &graphicsQueue);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        return -4;
    }

    Swapchain swapchain;
    //VkSwapchainKHR swapchain;
    if (createSwapchain(physicalDevice, device, surface, swapchain) != VK_SUCCESS) {
        return -5;
    }

    std::vector<VkImage> swapchainImages;
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain.swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain.swapchain, &imageCount, swapchainImages.data());

    std::vector<VkImageView> swapchainImageViews(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchain.swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            return -6;
        }
    }

    // window loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
    cleanup(window, surface, device, instance);

    glfwTerminate();
    return 0;
}

