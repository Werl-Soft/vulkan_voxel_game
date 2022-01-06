//
// Created by peter on 2021-12-22.
//

#ifndef BASIC_TESTS_VULKAN_HELPERS_HPP
#define BASIC_TESTS_VULKAN_HELPERS_HPP

#include <vulkan/vulkan.hpp>
#include <optional>
#include <set>

#include "GLFW/glfw3.h"

namespace game::vk_helpers {
    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool isComplete () const {
            return graphicsFamily.has_value () && presentFamily.has_value ();
        }
    };

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat (const std::vector<vk::SurfaceFormatKHR> &availableFormats);

    vk::PresentModeKHR chooseSwapPresentMode (const std::vector<vk::PresentModeKHR>& availablePresentModes);

    vk::Extent2D chooseSwapExtent (const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window, int *width, int *height);

    SwapChainSupportDetails querySwapChainSupport (const vk::PhysicalDevice &device, vk::SurfaceKHR surface);

    bool isDeviceSuitable (vk::PhysicalDevice device, vk::SurfaceKHR surface);

    bool checkDeviceExtensionSupport (vk::PhysicalDevice device);

    QueueFamilyIndices findQueueFamilies (vk::PhysicalDevice device, vk::SurfaceKHR surface);

    std::vector<const char *> getRequiredExtensions ();

    bool checkValidationLayerSupport ();

    vk::UniqueShaderModule createShaderModule (const std::string& path, vk::UniqueDevice &device);

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice device);

    void createBuffer(vk::DeviceSize size, vk::Flags<vk::BufferUsageFlagBits> usage, vk::Flags<vk::MemoryPropertyFlagBits> properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory,
                      vk::UniqueDevice &device,vk::PhysicalDevice &physicalDevice);

    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandPool commandPool, vk::UniqueDevice &device, vk::Queue graphicsQueue);
}

#endif //BASIC_TESTS_VULKAN_HELPERS_HPP
