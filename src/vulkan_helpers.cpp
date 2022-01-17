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
    auto supportedFeatures = device.getFeatures ();

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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
    auto code = file_loader::readFileBinary (path);

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
                                     vk::BufferUsageFlags usage,
                                     vk::MemoryPropertyFlags properties,
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
    auto commandBuffer = vk_helpers::beginSingleTimeCommands (commandPool, device);
    {
        vk::BufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        commandBuffer.copyBuffer (srcBuffer, dstBuffer, 1, &copyRegion);
    }
    vk_helpers::endSingleTimeCommands (commandBuffer, commandPool, device, graphicsQueue);
}

vk::UniqueImage game::vk_helpers::createImage (uint32_t width,
                                    uint32_t height,
                                    vk::Format format,
                                    vk::ImageTiling tiling,
                                    vk::Flags<vk::ImageUsageFlagBits> usage,
                                    vk::MemoryPropertyFlags properties,
                                    vk::UniqueDeviceMemory &imageMemory,
                                    vk::UniqueDevice &device,
                                    vk::PhysicalDevice &physicalDevice) {
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.height = height;
    imageInfo.extent.width = width;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;

    vk::UniqueImage image;

    try {
        image = device->createImageUnique(imageInfo);
    } catch (vk::SystemError &err) {
        spdlog::critical ("Failed to create image: {}", err.what());
        throw std::runtime_error ("Failed to create image!");
    }

    vk::MemoryRequirements memRequirements = device->getImageMemoryRequirements (image.get());

    vk::MemoryAllocateInfo allocateInfo = {};
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType (memRequirements.memoryTypeBits, properties, physicalDevice);

    try {
        imageMemory = device->allocateMemoryUnique (allocateInfo);
    } catch (vk::SystemError &err) {
        spdlog::critical ("Failed to allocate image memory: {}", err.what());
        throw std::runtime_error("Failed to allocate image memory!");
    }

    device->bindImageMemory (image.get(), imageMemory.get(), 0);


    return image;
}

vk::CommandBuffer game::vk_helpers::beginSingleTimeCommands (vk::CommandPool &commandPool, vk::UniqueDevice &device) {
    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    auto commandBuffer = device->allocateCommandBuffers (allocInfo);

    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer[0].begin (beginInfo);
    return commandBuffer[0];
}

void game::vk_helpers::endSingleTimeCommands (vk::CommandBuffer commandBuffer, vk::CommandPool &commandPool, vk::UniqueDevice &device, vk::Queue &graphicsQueue) {
    commandBuffer.end ();

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    graphicsQueue.submit (submitInfo, nullptr);
    graphicsQueue.waitIdle ();

    device->freeCommandBuffers (commandPool, commandBuffer);
}

void game::vk_helpers::transitionImageLayout (vk::Image image,
                                              vk::Format format,
                                              vk::ImageLayout oldLayout,
                                              vk::ImageLayout newLayout,
                                              vk::CommandPool &commandPool,
                                              vk::UniqueDevice &device,
                                              vk::Queue &graphicsQueue) {
    auto commandBuffer = beginSingleTimeCommands (commandPool, device);

    vk::ImageMemoryBarrier barrier = {};
    barrier.oldLayout = oldLayout;
    barrier.newLayout= newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal){
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }

    commandBuffer.pipelineBarrier (sourceStage, destinationStage,
                                   vk::DependencyFlagBits::eByRegion,
                                   nullptr,
                                   nullptr,
                                   barrier);

    endSingleTimeCommands (commandBuffer, commandPool, device, graphicsQueue);
}

void game::vk_helpers::copyBufferToImage (vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, vk::CommandPool &commandPool, vk::UniqueDevice &device, vk::Queue &graphicsQueue) {
    auto commandBuffer = beginSingleTimeCommands (commandPool, device);

    vk::BufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D(0, 0, 0);
    region.imageExtent = vk::Extent3D(width, height, 1);

    commandBuffer.copyBufferToImage (buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    endSingleTimeCommands (commandBuffer, commandPool, device, graphicsQueue);
}

vk::UniqueImageView game::vk_helpers::createImageView (vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::UniqueDevice &device) {
    vk::ImageViewCreateInfo createInfo = {};
    createInfo.image = image;
    createInfo.viewType = vk::ImageViewType::e2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    try {
        return device->createImageViewUnique(createInfo);
    } catch (vk::SystemError &err) {
        spdlog::critical ("Failed to create texture image view: {}", err.what());
        throw std::runtime_error("Failed to create texture image view!");
    }
}

vk::Format game::vk_helpers::findSupportedFormat (const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, vk::PhysicalDevice device) {

    for (vk::Format format : candidates) {
        vk::FormatProperties props = device.getFormatProperties (format);

        if(tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    spdlog::critical ("Failed to find supported format!");
    throw std::runtime_error("Failed to find supported format!");
}

vk::Format game::vk_helpers::findDepthFormat (vk::PhysicalDevice device) {
    return findSupportedFormat ({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                vk::ImageTiling::eOptimal,
                                vk::FormatFeatureFlagBits::eDepthStencilAttachment,
                                device);
}

bool game::vk_helpers::hasStencilComponent (vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}
