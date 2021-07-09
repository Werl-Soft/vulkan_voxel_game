#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>
#include <set>

const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
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

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class HelloTriangleApplication {
public:
    void run() {
        initWindow ();
        initVulkan ();
        mainLoop ();
        cleanup ();
    }

private:
    // Misc
    std::string appName = "Hello Triangle";
    std::string engineName = "Werlsoft Engine";

    // Window
    GLFWwindow* window;
    uint32_t width = 800;
    uint32_t height = 600;

    //Vulkan
    vk::UniqueInstance instance;
    vk::DispatchLoaderDynamic dld;
    VkDebugUtilsMessengerEXT callback;
    vk::SurfaceKHR surface;

    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice device;

    vk::Queue graphicsQueue;
    vk::Queue presentQueue;


    void initWindow() {
        glfwInit();

        glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow (width, height, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        setupDebugCallback();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void mainLoop() {
        while(! glfwWindowShouldClose (window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        // NOTE: instance destruction is handled by UniqueInstance, same for device

        // Surface is created by GLFW, therefore not using a Unique handle
        instance->destroySurfaceKHR (surface);

        glfwDestroyWindow (window);
        glfwTerminate();
    }

    /****************
     * Vulkan Stuff *
     ****************/
    /**
     * Creates the Vulkan Instance
     */
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        vk::ApplicationInfo appInfo(
                appName.c_str(),
                VK_MAKE_VERSION(0,1,0),
                engineName.c_str(),
                VK_MAKE_VERSION(0,1,0),
                VK_API_VERSION_1_0);

        auto extensions = getRequiredExtensions();

        auto createInfo = vk::InstanceCreateInfo (
                vk::InstanceCreateFlags(),
                &appInfo,
                0, nullptr, // enabled layers
                static_cast<uint32_t>(extensions.size()), extensions.data()); // enabled extensions

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        try {
            instance = vk::createInstanceUnique(createInfo, nullptr);
        }catch (vk::SystemError err) {
            throw std::runtime_error("Failed to create instance");
        }

        spdlog::get ("console")->info ("Available Extensions:");

        for (const auto& extension : vk::enumerateInstanceExtensionProperties ()) {
            spdlog::get ("console")->info ("\t {}", extension.extensionName);
        }

        dld = vk::DispatchLoaderDynamic(instance->operator VkInstance_T *(), vkGetInstanceProcAddr);
    }

    void createSurface() {
        VkSurfaceKHR rawSurface;
        if (glfwCreateWindowSurface (instance->operator VkInstance_T * (), window, nullptr, &rawSurface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
        surface = static_cast<vk::SurfaceKHR>(rawSurface);
    }

    void setupDebugCallback() {
        if (! enableValidationLayers)
            return;

        auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT (
                vk::DebugUtilsMessengerCreateFlagsEXT (),
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                debugCallback,
                nullptr);

        /*if (CreateDebugUtilsMessengerEXT (instance->operator VkInstance_T * (), reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT *>(&createInfo), nullptr, &callback) != VK_SUCCESS) {
            throw std::runtime_error ("failed to set up debug callback!");
        }*/

        instance->createDebugUtilsMessengerEXTUnique(createInfo, nullptr, dld);
    }

    void pickPhysicalDevice() {
        auto devices = instance->enumeratePhysicalDevices();
        if (devices.size() == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (!physicalDevice) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };


        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            queueCreateInfos.push_back({
                vk::DeviceQueueCreateFlags(),
                queueFamily,
                1, // queueCount
                &queuePriority
            });
        }

        auto deviceFeatures = vk::PhysicalDeviceFeatures();
        auto createInfo = vk::DeviceCreateInfo(
                vk::DeviceCreateFlags(),
                static_cast<uint32_t>(queueCreateInfos.size()),
                queueCreateInfos.data()
                );
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        try {
            device = physicalDevice.createDeviceUnique(createInfo);
        } catch (vk::SystemError err) {
            throw std::runtime_error("failed to create logical device!");
        }

        graphicsQueue = device->getQueue(indices.graphicsFamily.value(), 0);
        presentQueue = device->getQueue (indices.presentFamily.value(), 0);
    }

    // Helper Functions

    bool isDeviceSuitable(const vk::PhysicalDevice& device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        return indices.isComplete();
    }

    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
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

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);

        std::vector<const char*> extensions (glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkValidationLayerSupport() {
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

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    auto stdLogger = spdlog::stdout_color_mt ("console");

    try {
        stdLogger->info ("Starting Application");
        app.run ();
        stdLogger->info ("Stopping Application");
    } catch (std::exception& e) {
        stdLogger->critical ("Application Crashed: {}", e.what ());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
