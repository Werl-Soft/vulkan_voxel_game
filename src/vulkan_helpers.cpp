//
// Created by peter on 2021-12-22.
//

#include "vulkan_helpers.hpp"
#include "file_loader.hpp"
#include <spdlog/spdlog.h>

vk::SurfaceFormatKHR game::vk_helpers::chooseSwapSurfaceFormat (const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
        return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR game::vk_helpers::chooseSwapPresentMode (const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        } else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

vk::Extent2D game::vk_helpers::chooseSwapExtent (const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window, int *width, int *height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()){
        return capabilities.currentExtent;
    } else {
        glfwGetFramebufferSize (window, width, height);

        vk::Extent2D actualExtent = { static_cast<uint32_t>(*width), static_cast<uint32_t>(*height) };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

game::vk_helpers::SwapChainSupportDetails game::vk_helpers::querySwapChainSupport(const vk::PhysicalDevice& device, vk::SurfaceKHR surface) {
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}

bool game::vk_helpers::isDeviceSuitable (vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport (device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool game::vk_helpers::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : device.enumerateDeviceExtensionProperties ()) {
        requiredExtensions.erase (extension.extensionName);
    }
    return requiredExtensions.empty();
}

game::vk_helpers::QueueFamilyIndices game::vk_helpers::findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    QueueFamilyIndices indices;

    auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }

        if (queueFamily.queueCount > 0 && device.getSurfaceSupportKHR (i, surface)) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

std::vector<const char *> game::vk_helpers::getRequiredExtensions () {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);

    std::vector<const char*> extensions (glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool game::vk_helpers::checkValidationLayerSupport () {
    auto availableLayers = vk::enumerateInstanceLayerProperties ();
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp (layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
            return false;
    }
    return true;
}

vk::UniqueShaderModule game::vk_helpers::createShaderModule (const std::string& path, vk::UniqueDevice &device) {
    auto code = file_loader::readFile (path);

    try {
        return device->createShaderModuleUnique ({vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t*>(code.data())});
    } catch (vk::SystemError& error) {
        spdlog::get ("main_out")->critical ("Failed to create shader module: {}", error.what());
        throw std::runtime_error ("failed to create shader module!");
    }
}

uint32_t game::vk_helpers::findMemoryType (uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice device) {
    vk::PhysicalDeviceMemoryProperties memoryProperties = device.getMemoryProperties ();

    for (uint32_t i = 0; i <memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    spdlog::get ("main_out")->critical ("Failed to find suitable memory type!");
    throw std::runtime_error("Failed to find suitable memory type!");
}

void game::vk_helpers::createBuffer (vk::DeviceSize size,
                                     vk::Flags<vk::BufferUsageFlagBits> usage,
                                     vk::Flags<vk::MemoryPropertyFlagBits> properties,
                                     vk::Buffer &buffer,
                                     vk::DeviceMemory &bufferMemory,
                                     vk::UniqueDevice &device,
                                     vk::PhysicalDevice &physicalDevice) {
    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.usage = usage;
    bufferInfo.size = size;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    try {
        buffer = device->createBuffer (bufferInfo);
    } catch (vk::SystemError &err) {
        spdlog::get ("main_out")->critical ("Failed to create buffer: {}", err.what());
        throw std::runtime_error ("Failed to create buffer!");
    }

    auto memRequirements = device->getBufferMemoryRequirements (buffer);

    vk::MemoryAllocateInfo memInfo = {};
    memInfo.allocationSize = memRequirements.size;
    memInfo.memoryTypeIndex = vk_helpers::findMemoryType (memRequirements.memoryTypeBits, properties, physicalDevice);
    try{
        bufferMemory = device->allocateMemory (memInfo);
    } catch (vk::SystemError &err) {
        spdlog::get ("main_out")->critical ("Failed to allocate buffer memory; {}", err.what());
        throw std::runtime_error("Failed to allocate vertex memory!");
    }

    device->bindBufferMemory (buffer, bufferMemory, 0);
}

void game::vk_helpers::copyBuffer (vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandPool commandPool, vk::UniqueDevice &device, vk::Queue graphicsQueue) {
    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    auto commandBuffer = device->allocateCommandBuffers (allocInfo);

    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer[0].begin (beginInfo);
    {
        vk::BufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        commandBuffer[0].copyBuffer (srcBuffer, dstBuffer, 1, &copyRegion);
    }
    commandBuffer[0].end ();

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = commandBuffer.data();

    graphicsQueue.submit (submitInfo, nullptr);
    graphicsQueue.waitIdle ();

    device->freeCommandBuffers (commandPool, commandBuffer);
}
