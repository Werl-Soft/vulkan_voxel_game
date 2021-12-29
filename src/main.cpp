#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>
#include <set>
#include <memory>
#include <chrono>

#include "vulkan_helpers.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

namespace game {
    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;

        static vk::VertexInputBindingDescription getBindingDescription () {
            vk::VertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof (Vertex);
            bindingDescription.inputRate = vk::VertexInputRate::eVertex;

            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription () {
            std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = {};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            return attributeDescriptions;
        }
    };

    const std::vector<Vertex> vertices = {
            {{-0.5f, - 0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f},   {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f},   {1.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f},    {0.0f, 0.0f, 1.0f,}}
    };

    const std::vector<uint16_t> indices = {
            0, 1, 2, 2, 1, 3
    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    class HelloTriangleApplication {
    public:
        void run () {
            logger = spdlog::get ("main_out");
            initWindow ();
            initVulkan ();
            mainLoop ();
            cleanup ();
        }

    private:
        // Misc
        std::string appName = "Hello Triangle";
        std::string engineName = "Werlsoft Engine";
        std::shared_ptr<spdlog::logger> logger;

        // Window
        GLFWwindow *window{};
        int32_t width = 800;
        int32_t height = 600;

        int maxFramesInFlight = 2;

        //Vulkan
        vk::UniqueInstance instance;
        vk::DispatchLoaderDynamic dld;
        //VkDebugUtilsMessengerEXT callback;
        vk::SurfaceKHR surface;

        vk::PhysicalDevice physicalDevice;
        vk::UniqueDevice device;

        vk::Queue graphicsQueue;
        vk::Queue presentQueue;

        vk::SwapchainKHR swapChain;
        std::vector<vk::Image> swapChainImages;
        vk::Format swapChainImageFormat;
        vk::Extent2D swapChainExtent;
        std::vector<vk::ImageView> swapChainImageViews;

        vk::RenderPass renderPass;
        vk::DescriptorSetLayout descriptorSetLayout;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline graphicsPipeline;

        std::vector<vk::Framebuffer> swapChainFrameBuffers;

        vk::CommandPool commandPool;
        std::vector<vk::CommandBuffer, std::allocator<vk::CommandBuffer>> commandBuffers;

        std::vector<vk::Semaphore> imageAvailableSemaphores;
        std::vector<vk::Semaphore> renderFinishedSemaphores;
        std::vector<vk::Fence> inFlightFences;
        size_t currentFrame = 0;

        vk::Buffer vertexBuffer;
        vk::DeviceMemory vertexBufferMemory;
        vk::Buffer indexBuffer;
        vk::DeviceMemory indexBufferMemory;

        std::vector<vk::Buffer> uniformBuffers;
        std::vector<vk::DeviceMemory> uniformBufferMemory;

        vk::DescriptorPool descriptorPool;
        std::vector<vk::DescriptorSet> descriptorSets;

    public:
        bool frameBufferResized = false;

    private:
        void initWindow () {
            logger->info ("Starting: GLFW Initialization");

            glfwInit ();

            glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);

            window = glfwCreateWindow (width, height, "Vulkan", nullptr, nullptr);
            glfwSetWindowUserPointer (window, this);
            glfwSetFramebufferSizeCallback (window, frameBufferResizeCallback);

            logger->info ("Finished: GLFW Initialization");
        }

        static void frameBufferResizeCallback (GLFWwindow *window, int width, int height) {
            auto app = reinterpret_cast<HelloTriangleApplication *>(glfwGetWindowUserPointer (window));
            app->frameBufferResized = true;
        }

        void initVulkan () {
            logger->info ("Starting: Vulkan Initialization");
            createInstance ();
            setupDebugCallback ();
            createSurface ();
            pickPhysicalDevice ();
            createLogicalDevice ();
            createSwapChain ();
            createImageViews ();
            createRenderPass ();
            createDescriptorSetLayout();
            createGraphicsPipeline ();
            createFrameBuffers ();
            createCommandPool ();
            createVertexBuffer ();
            createIndexBuffer();
            createUniformBuffers();
            createDescriptorPool();
            createDescriptorSets();
            createCommandBuffers ();
            createSyncObjects ();

            logger->info ("Finished: Vulkan Initialization");
        }

        void mainLoop () {
            logger->info ("Starting: Main Loop");
            while (! glfwWindowShouldClose (window)) {
                glfwPollEvents ();
                drawFrame ();
            }

            device->waitIdle ();
            logger->info ("Finished: Main Loop");
        }

        void cleanupSwapChain () {
            logger->info ("Starting: Swap Chain Cleanup");

            for (auto frameBuffer: swapChainFrameBuffers) {
                device->destroyFramebuffer (frameBuffer);
            }

            device->freeCommandBuffers (commandPool, commandBuffers);

            device->destroyPipeline (graphicsPipeline);
            device->destroyRenderPass (renderPass);
            device->destroyPipelineLayout (pipelineLayout);

            for (auto imageView: swapChainImageViews) {
                device->destroyImageView (imageView);
            }
            device->destroySwapchainKHR (swapChain);

            for (size_t i = 0; i < swapChainImages.size(); i++) {
                device->destroyBuffer (uniformBuffers[i]);
                device->freeMemory (uniformBufferMemory[i]);
            }

            device->destroyDescriptorPool (descriptorPool);

            logger->info ("Finished: Swap Chain Cleanup");
        }

        void cleanup () {
            // NOTE: instance destruction is handled by UniqueInstance, same for device
            logger->info ("Starting: Cleanup");

            cleanupSwapChain ();

            device->destroyDescriptorSetLayout (descriptorSetLayout);

            device->destroyBuffer (vertexBuffer);
            device->freeMemory (vertexBufferMemory);
            device->destroyBuffer (indexBuffer);
            device->freeMemory (indexBufferMemory);

            for (size_t i = 0; i < maxFramesInFlight; i ++) {
                device->destroySemaphore (renderFinishedSemaphores[i]);
                device->destroySemaphore (imageAvailableSemaphores[i]);
                device->destroyFence (inFlightFences[i]);
            }

            device->destroyCommandPool (commandPool);

            // Surface is created by GLFW, therefore not using a Unique handle
            instance->destroySurfaceKHR (surface);

            glfwDestroyWindow (window);
            glfwTerminate ();

            logger->info ("Finished: Cleanup");
        }

        /****************
         * Vulkan Stuff *
         ****************/
        /**
         * Creates the Vulkan Instance
         */
        void recreateSwapChain () {
            logger->info ("Starting: Swap Chain Recreation");
            width = 0;
            height = 0;
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize (window, &width, &height);
                glfwWaitEvents ();
            }

            device->waitIdle ();

            cleanupSwapChain ();

            createSwapChain ();
            createImageViews ();
            createRenderPass ();
            createGraphicsPipeline ();
            createFrameBuffers ();
            createUniformBuffers();
            createDescriptorPool();
            createDescriptorSets();
            createCommandBuffers ();

            logger->info ("Finished: Swap Chain Recreation");
        }

        void createInstance () {
            logger->info ("Starting: Vulkan Instance Creation");
            if (vk_helpers::enableValidationLayers && ! vk_helpers::checkValidationLayerSupport ()) {
                logger->critical ("Validation layers requested, but not available!");
                throw std::runtime_error ("Validation layers requested, but not available!");
            }

            vk::ApplicationInfo appInfo (
                    appName.c_str (),
                    VK_MAKE_VERSION(0, 1, 0),
                    engineName.c_str (),
                    VK_MAKE_VERSION(0, 1, 0),
                    VK_API_VERSION_1_0);

            auto extensions = vk_helpers::getRequiredExtensions ();

            auto createInfo = vk::InstanceCreateInfo (
                    vk::InstanceCreateFlags (),
                    &appInfo,
                    0, nullptr, // enabled layers
                    static_cast<uint32_t>(extensions.size ()), extensions.data ()); // enabled extensions

            if (vk_helpers::enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size ());
                createInfo.ppEnabledLayerNames = validationLayers.data ();
            }

            try {
                instance = vk::createInstanceUnique (createInfo, nullptr);
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to create instance: {}", err.what());
                throw std::runtime_error ("Failed to create instance");
            }

            logger->info ("Available Extensions:");

            for (const auto &extension: vk::enumerateInstanceExtensionProperties ()) {
                logger->info ("\t {}", extension.extensionName);
            }

            dld = vk::DispatchLoaderDynamic (instance->operator VkInstance_T * (), vkGetInstanceProcAddr);

            logger->info ("Finished: Vulkan Instance Creation");
        }

        void createSurface () {
            logger->info ("Starting: Vulkan Surface Creation");
            VkSurfaceKHR rawSurface;
            if (glfwCreateWindowSurface (instance->operator VkInstance_T * (), window, nullptr, &rawSurface) != VK_SUCCESS) {
                logger->critical ("Failed to create window surface!");
                throw std::runtime_error ("Failed to create window surface!");
            }
            surface = static_cast<vk::SurfaceKHR>(rawSurface);

            logger->info ("Finished: Vulkan Surface Creation");
        }

        void setupDebugCallback () {
            if (! vk_helpers::enableValidationLayers)
                return;
            logger->info ("Starting: Debug Callback Creation");

            auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT (
                    vk::DebugUtilsMessengerCreateFlagsEXT (),
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                    debugCallback,
                    nullptr);

            /*if (CreateDebugUtilsMessengerEXT (instance->operator VkInstance_T * (), reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT *>(&createInfo), nullptr, &callback) != VK_SUCCESS) {
                throw std::runtime_error ("failed to set up debug callback!");
            }*/

            instance->createDebugUtilsMessengerEXTUnique (createInfo, nullptr, dld);

            logger->info ("Finished: Debug Callback Creation");
        }

        void pickPhysicalDevice () {
            logger->info ("Starting: Choose Physical Device");
            auto devices = instance->enumeratePhysicalDevices ();
            if (devices.empty ()) {
                logger->critical ("Failed to find GPUs with Vulkan support!");
                throw std::runtime_error ("Failed to find GPUs with Vulkan support!");
            }

            for (const auto &d: devices) {
                if (vk_helpers::isDeviceSuitable (d, surface)) {
                    physicalDevice = d;
                    break;
                }
            }

            if (! physicalDevice) {
                logger->critical ("Failed to find a suitable GPU!");
                throw std::runtime_error ("Failed to find a suitable GPU!");
            }

            logger->info ("Finished: Choose Physical Device");
        }

        void createLogicalDevice () {
            logger->info ("Starting: Logical Device Creation");
            vk_helpers::QueueFamilyIndices indices = vk_helpers::findQueueFamilies (physicalDevice, surface);

            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value (), indices.presentFamily.value ()};

            float queuePriority = 1.0f;

            queueCreateInfos.reserve (uniqueQueueFamilies.size ());
            for (uint32_t queueFamily: uniqueQueueFamilies) {
                queueCreateInfos.emplace_back (vk::DeviceQueueCreateFlags (), queueFamily, 1, &queuePriority);
            }

            auto deviceFeatures = vk::PhysicalDeviceFeatures ();
            auto createInfo = vk::DeviceCreateInfo (
                    vk::DeviceCreateFlags (),
                    static_cast<uint32_t>(queueCreateInfos.size ()),
                    queueCreateInfos.data ());
            createInfo.pEnabledFeatures = &deviceFeatures;
            createInfo.enabledExtensionCount = deviceExtensions.size ();
            createInfo.ppEnabledExtensionNames = deviceExtensions.data ();

            if (vk_helpers::enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size ());
                createInfo.ppEnabledLayerNames = validationLayers.data ();
            }

            try {
                device = physicalDevice.createDeviceUnique (createInfo);
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to create logical device: {}", err.what());
                throw std::runtime_error ("failed to create logical device!");
            }

            graphicsQueue = device->getQueue (indices.graphicsFamily.value (), 0);
            presentQueue = device->getQueue (indices.presentFamily.value (), 0);

            logger->info ("Finished: Logical Device Creation");
        }

        void createSwapChain () {
            logger->info ("Starting: Swap Chain Creation");
            vk_helpers::SwapChainSupportDetails swapChainSupport = vk_helpers::querySwapChainSupport (physicalDevice, surface);

            vk::SurfaceFormatKHR surfaceFormat = vk_helpers::chooseSwapSurfaceFormat (swapChainSupport.formats);
            vk::PresentModeKHR presentMode = vk_helpers::chooseSwapPresentMode (swapChainSupport.presentModes);
            vk::Extent2D extent = vk_helpers::chooseSwapExtent (swapChainSupport.capabilities, window, &width, &height);

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
                imageCount = swapChainSupport.capabilities.maxImageCount;
            }

            vk::SwapchainCreateInfoKHR createInfo (
                    vk::SwapchainCreateFlagsKHR (),
                    surface,
                    imageCount,
                    surfaceFormat.format,
                    surfaceFormat.colorSpace,
                    extent,
                    1, // imageArrayLayers
                    vk::ImageUsageFlagBits::eColorAttachment);

            vk_helpers::QueueFamilyIndices indices = vk_helpers::findQueueFamilies (physicalDevice, surface);
            uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value (), indices.presentFamily.value ()};

            if (indices.graphicsFamily != indices.presentFamily) {
                createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                createInfo.imageSharingMode = vk::SharingMode::eExclusive;
            }

            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;

            createInfo.oldSwapchain = vk::SwapchainKHR (nullptr);

            try {
                swapChain = device->createSwapchainKHR (createInfo);
            }
            catch (vk::SystemError &err) {
                logger->critical ("Failed to create swap chain: {}", err.what());
                throw std::runtime_error ("failed to create swap chain!");
            }

            swapChainImages = device->getSwapchainImagesKHR (swapChain);

            swapChainImageFormat = surfaceFormat.format;
            swapChainExtent = extent;
            logger->info ("Finished: Swap Chain Creation");
        }

        void createImageViews () {
            logger->info ("Starting: Image View Creation");
            swapChainImageViews.resize (swapChainImages.size ());

            for (size_t i = 0; i < swapChainImages.size (); i ++) {
                vk::ImageViewCreateInfo createInfo = {};
                createInfo.image = swapChainImages[i];
                createInfo.viewType = vk::ImageViewType::e2D;
                createInfo.format = swapChainImageFormat;
                createInfo.components.r = vk::ComponentSwizzle::eIdentity;
                createInfo.components.g = vk::ComponentSwizzle::eIdentity;
                createInfo.components.b = vk::ComponentSwizzle::eIdentity;
                createInfo.components.a = vk::ComponentSwizzle::eIdentity;
                createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                try {
                    swapChainImageViews[i] = device->createImageView (createInfo);
                }
                catch (vk::SystemError &err) {
                    logger->critical ("Failed to create image views: {}", err.what());
                    throw std::runtime_error ("failed to create image views!");
                }
            }
            logger->info ("Finished: Image View Creation");
        }

        void createRenderPass () {
            logger->info ("Starting: Render Pass Creation");

            vk::AttachmentDescription colorAttachment = {};
            colorAttachment.format = swapChainImageFormat;
            colorAttachment.samples = vk::SampleCountFlagBits::e1;
            colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
            colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
            colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
            colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

            vk::AttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            vk::SubpassDescription subpass{};
            subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            vk::SubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

            dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

            vk::RenderPassCreateInfo renderPassInfo{};
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            try {
                renderPass = device->createRenderPass (renderPassInfo);
            } catch (vk::SystemError &err) {
                logger->critical("Failed to create render pass: {}", err.what());
                throw std::runtime_error ("Failed to create render pass!");
            }
            logger->info ("Finished: Render Pass Creation");
        }

        void createDescriptorSetLayout() {
            vk::DescriptorSetLayoutBinding uboLayoutBinding = {};
            uboLayoutBinding.binding = 0;
            uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
            uboLayoutBinding.pImmutableSamplers = nullptr;

            vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &uboLayoutBinding;

            try {
                descriptorSetLayout = device->createDescriptorSetLayout (layoutInfo);
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to create descriptor set layout: {}", err.what());
                throw std::runtime_error("Failed to create descriptor set layout");
            }
        }

        void createGraphicsPipeline () {
            logger->info ("Starting: Graphics Pipeline Creation");

            auto vertShaderModule = vk_helpers::createShaderModule ("assets/shaders/vert.spv", device);
            auto fragShaderModule = vk_helpers::createShaderModule ("assets/shaders/frag.spv", device);

            vk::PipelineShaderStageCreateInfo shaderStages[] = {
                    {
                            vk::PipelineShaderStageCreateFlags (),
                            vk::ShaderStageFlagBits::eVertex,
                            *vertShaderModule,
                            "main"
                    },
                    {
                            vk::PipelineShaderStageCreateFlags (),
                            vk::ShaderStageFlagBits::eFragment,
                            *fragShaderModule,
                            "main"
                    }
            };

            auto bindingDescription = Vertex::getBindingDescription ();
            auto attributeDescriptions = Vertex::getAttributeDescription ();

            vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size ());
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data ();

            vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
            inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            vk::Viewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float) swapChainExtent.width;
            viewport.height = (float) swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            vk::Rect2D scissor{};
            scissor.offset = vk::Offset2D (0, 0);
            scissor.extent = swapChainExtent;

            vk::PipelineViewportStateCreateInfo viewportState = {};
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;

            vk::PipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = vk::PolygonMode::eFill;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = vk::CullModeFlagBits::eBack;
            rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
            rasterizer.depthBiasEnable = VK_FALSE;

            vk::PipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

            vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
            colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            colorBlendAttachment.blendEnable = VK_FALSE;

            vk::PipelineColorBlendStateCreateInfo colorBlending = {};
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = vk::LogicOp::eCopy;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f;
            colorBlending.blendConstants[1] = 0.0f;
            colorBlending.blendConstants[2] = 0.0f;
            colorBlending.blendConstants[3] = 0.0f;

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 0;

            try {
                pipelineLayout = device->createPipelineLayout (pipelineLayoutInfo);
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to create pipeline layout: {}", err.what());
                throw std::runtime_error ("Failed to create pipeline layout!");
            }

            vk::GraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.renderPass = renderPass;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = nullptr;

            try {
                graphicsPipeline = device->createGraphicsPipeline (nullptr, pipelineInfo).value;
            }
            catch (vk::SystemError &err) {
                logger->critical ("Failed to create graphics pipeline: {}", err.what());
                throw std::runtime_error ("failed to create graphics pipeline!");
            }
            logger->info ("Finished: Graphics Pipeline Creation");
        }

        void createFrameBuffers () {
            logger->info ("Starting: Frame Buffer Creation");

            swapChainFrameBuffers.resize (swapChainImageViews.size ());

            for (size_t i = 0; i < swapChainImageViews.size (); i ++) {
                vk::ImageView attachments[] = {
                        swapChainImageViews[i]
                };

                vk::FramebufferCreateInfo framebufferCreateInfo = {};
                framebufferCreateInfo.renderPass = renderPass;
                framebufferCreateInfo.attachmentCount = 1;
                framebufferCreateInfo.pAttachments = attachments;
                framebufferCreateInfo.width = swapChainExtent.width;
                framebufferCreateInfo.height = swapChainExtent.height;
                framebufferCreateInfo.layers = 1;

                try {
                    swapChainFrameBuffers[i] = device->createFramebuffer (framebufferCreateInfo);
                } catch (vk::SystemError &err) {
                    logger->critical ("Failed to create frame buffer: {}", err.what());
                    throw std::runtime_error ("Failed to create frame buffer");
                }
            }

            logger->info ("Finished: Frame Buffer Creation");
        }

        void createCommandPool () {
            logger->info ("Starting: Command Pool Creation");

            vk_helpers::QueueFamilyIndices queueFamilyIndices = vk_helpers::findQueueFamilies (physicalDevice, surface);

            vk::CommandPoolCreateInfo poolInfo = {};
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value ();

            try {
                commandPool = device->createCommandPool (poolInfo);
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to create command pool: {}", err.what());
                throw std::runtime_error ("Failed to create command pool!");
            }

            logger->info ("Finished: Command Pool Creation");
        }

        void createVertexBuffer () {
            logger->info ("Starting: Vertex Buffer Creation");

            vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

            vk::Buffer stagingBuffer;
            vk::DeviceMemory stagingBufferMemory;
            vk_helpers::createBuffer (bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                      stagingBuffer, stagingBufferMemory, device, physicalDevice);

            void* data = device->mapMemory (stagingBufferMemory, 0, bufferSize);
            {
                memcpy (data, vertices.data (), bufferSize);
            }
            device->unmapMemory (stagingBufferMemory);

            vk_helpers::createBuffer (bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                      vertexBuffer, vertexBufferMemory, device, physicalDevice);

            vk_helpers::copyBuffer (stagingBuffer, vertexBuffer, bufferSize, commandPool, device, graphicsQueue);

            device->destroyBuffer (stagingBuffer);
            device->freeMemory (stagingBufferMemory);

            logger->info ("Finished: Vertex Buffer Creation");
        }

        void createIndexBuffer() {
            logger->info ("Starting: Index Buffer Creation");

            vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

            vk::Buffer stagingBuffer;
            vk::DeviceMemory stagingBufferMemory;
            vk_helpers::createBuffer (bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                      stagingBuffer, stagingBufferMemory, device, physicalDevice);

            void* data = device->mapMemory (stagingBufferMemory, 0, bufferSize);
            {
                memcpy (data, indices.data (), bufferSize);
            }
            device->unmapMemory (stagingBufferMemory);

            vk_helpers::createBuffer (bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                      indexBuffer, indexBufferMemory, device, physicalDevice);

            vk_helpers::copyBuffer (stagingBuffer, indexBuffer, bufferSize, commandPool, device, graphicsQueue);

            device->destroyBuffer (stagingBuffer);
            device->freeMemory (stagingBufferMemory);

            logger->info ("Finished: Index Buffer Creation");
        }

        void createUniformBuffers() {
            logger->info ("Starting: Uniform Buffer Creation");

            vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

            uniformBuffers.resize(swapChainImages.size());
            uniformBufferMemory.resize (swapChainImages.size());

            for (size_t i = 0; i < swapChainImages.size(); i++) {
                vk_helpers::createBuffer (bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                          uniformBuffers[i], uniformBufferMemory[i], device, physicalDevice);
            }

            logger->info ("Finished: Uniform Buffer Creation");
        }

        void createDescriptorPool() {
            logger->info ("Starting: Descriptor Pool Creation");

            vk::DescriptorPoolSize poolSize = {};
            poolSize.type = vk::DescriptorType::eUniformBuffer;
            poolSize.descriptorCount = static_cast<uint32_t>(swapChainImages.size());

            vk::DescriptorPoolCreateInfo poolInfo = {};
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

            try{
                descriptorPool = device->createDescriptorPool (poolInfo);
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to create descriptor pool: {}", err.what());
                throw std::runtime_error("Failed to create descriptor pool");
            }

            logger->info ("Finished: Descriptor Pool Creation");
        }

        void createDescriptorSets() {
            logger->info ("Starting: Descriptor Set Creation");

            std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
            vk::DescriptorSetAllocateInfo allocInfo = {};
            allocInfo.descriptorPool = descriptorPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
            allocInfo.pSetLayouts = layouts.data();

            descriptorSets.resize (swapChainImages.size());
            try {
                descriptorSets = device->allocateDescriptorSets (allocInfo);
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to allocate descriptor sets: {}", err.what());
                throw std::runtime_error("failed to allocate descriptor sets!");
            }

            for (size_t i = 0; i < swapChainImages.size(); i++) {
                vk::DescriptorBufferInfo bufferInfo = {};
                bufferInfo.buffer = uniformBuffers[i];
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof (UniformBufferObject);

                vk::WriteDescriptorSet descriptorWriter = {};
                descriptorWriter.dstSet = descriptorSets[i];
                descriptorWriter.dstBinding = 0;
                descriptorWriter.dstArrayElement = 0;
                descriptorWriter.descriptorType = vk::DescriptorType::eUniformBuffer;
                descriptorWriter.descriptorCount = 1;
                descriptorWriter.pBufferInfo = &bufferInfo;
                descriptorWriter.pImageInfo = nullptr;
                descriptorWriter.pTexelBufferView = nullptr;

                device->updateDescriptorSets (1, &descriptorWriter, 0, nullptr);
            }

            logger->info ("Finished: Descriptor Set Creation");
        }

        void createCommandBuffers () {
            logger->info ("Starting: Command Buffer Creation");

            commandBuffers.resize (swapChainFrameBuffers.size ());

            vk::CommandBufferAllocateInfo allocateInfo = {};
            allocateInfo.commandPool = commandPool;
            allocateInfo.level = vk::CommandBufferLevel::ePrimary;
            allocateInfo.commandBufferCount = (uint32_t) commandBuffers.size ();

            try {
                commandBuffers = device->allocateCommandBuffers (allocateInfo);
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to allocate command buffers: {}", err.what());
                throw std::runtime_error ("Failed to allocate command buffers!");
            }

            for (size_t i = 0; i < commandBuffers.size (); i ++) {
                vk::CommandBufferBeginInfo beginInfo = {};
                beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

                try {
                    commandBuffers[i].begin (beginInfo);
                } catch (vk::SystemError &err) {
                    logger->critical ("Failed to begin recording command buffer {} of {}: {}",i, commandBuffers.size(), err.what());
                    throw std::runtime_error ("Failed to begin recording command buffer!");
                }

                vk::RenderPassBeginInfo renderPassInfo = {};
                renderPassInfo.renderPass = renderPass;
                renderPassInfo.framebuffer = swapChainFrameBuffers[i];
                renderPassInfo.renderArea.offset = vk::Offset2D (0, 0);
                renderPassInfo.renderArea.extent = swapChainExtent;

                vk::ClearValue clearColor = {std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearColor;

                commandBuffers[i].beginRenderPass (renderPassInfo, vk::SubpassContents::eInline);
                {
                    commandBuffers[i].bindPipeline (vk::PipelineBindPoint::eGraphics, graphicsPipeline);
                    vk::Buffer vertexBuffers[] = {vertexBuffer};
                    vk::DeviceSize offsets[] = {0};
                    commandBuffers[i].bindVertexBuffers (0, 1, vertexBuffers, offsets);
                    commandBuffers[i].bindIndexBuffer (indexBuffer, 0, vk::IndexType::eUint16);
                    commandBuffers[i].bindDescriptorSets (vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[i], 0,nullptr);
                    commandBuffers[i].drawIndexed(static_cast<uint32_t>(indices.size ()), 1, 0, 0, 0);
                }
                commandBuffers[i].endRenderPass ();

                try {
                    commandBuffers[i].end ();
                } catch (vk::SystemError &err) {
                    logger->critical ("Failed to record command buffer {} of {}: {}", i, commandBuffers.size(), err.what());
                    throw std::runtime_error ("Failed to record command buffer!");
                }
            }

            logger->info ("Finished: Command Buffer Creation");
        }

        void createSyncObjects () {
            logger->info ("Starting: Sync Objects Creation");

            imageAvailableSemaphores.resize (maxFramesInFlight);
            renderFinishedSemaphores.resize (maxFramesInFlight);
            inFlightFences.resize (maxFramesInFlight);

            try {
                for (size_t i = 0; i < maxFramesInFlight; i ++) {
                    imageAvailableSemaphores[i] = device->createSemaphore ({});
                    renderFinishedSemaphores[i] = device->createSemaphore ({});
                    inFlightFences[i] = device->createFence ({vk::FenceCreateFlagBits::eSignaled});
                }
            } catch (vk::SystemError &err) {
                logger->critical ("Failed to create sync objects for a frame: {}", err.what());
                throw std::runtime_error ("Failed to create synchronization objects for a frame!");
            }

            logger->info ("Finished: Sync Objects Creation");
        }

        void drawFrame () {
            device->waitForFences (inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max ());
            device->resetFences (inFlightFences[currentFrame]);

            uint32_t imageIndex;
            try {
                vk::ResultValue result = device->acquireNextImageKHR (swapChain, std::numeric_limits<uint64_t>::max (),
                                                                      imageAvailableSemaphores[currentFrame], nullptr);
                imageIndex = result.value;
            } catch (vk::OutOfDateKHRError &err) {
                recreateSwapChain ();
                return;
            } catch (vk::SystemError &err) {
                spdlog::get ("main_out")->critical ("Failed to acquire swap chain image!");
                spdlog::get ("main_out")->dump_backtrace();
                throw std::runtime_error ("Failed to acquire swap chain image!");
            }

            updateUniformBuffer (imageIndex);

            vk::SubmitInfo submitInfo = {};
            vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
            vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

            vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            device->resetFences (inFlightFences[currentFrame]);

            try {
                graphicsQueue.submit (submitInfo, inFlightFences[currentFrame]);
            } catch (vk::SystemError &err) {
                throw std::runtime_error ("Failed to submit draw command buffer!");
            }

            vk::PresentInfoKHR presentInfo = {};
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            vk::SwapchainKHR swapchains[] = {swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &imageIndex;

            vk::Result resultPresent;
            try {
                resultPresent = presentQueue.presentKHR (presentInfo);
            } catch (vk::OutOfDateKHRError &err) {
                resultPresent = vk::Result::eErrorOutOfDateKHR;
            } catch (vk::SystemError &err) {
                throw std::runtime_error ("failed to present swap chain image!");
            }

            if (resultPresent == vk::Result::eSuboptimalKHR || frameBufferResized) {
                frameBufferResized = false;
                recreateSwapChain ();
                return;
            }

            currentFrame = (currentFrame + 1) % maxFramesInFlight;
        }

        void updateUniformBuffer(uint32_t currentImage) {
            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period> (currentTime - startTime).count();

            UniformBufferObject ubo = {};
            ubo.model = glm::rotate (glm::mat4(1.0f), time * glm::radians (90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.view = glm::lookAt (glm::vec3(2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.proj = glm::perspective (glm::radians (45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
            ubo.proj[1][1] *= -1;

            void* data = device->mapMemory (uniformBufferMemory[currentImage], 0, sizeof(ubo));
            {
                memcpy (data, &ubo, sizeof (ubo));
            }
            device->unmapMemory (uniformBufferMemory[currentImage]);
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
        }
    };
}

int main(int argc, char* argv[]) {
    game::HelloTriangleApplication app;

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level (spdlog::level::warn);

    auto max_size = 1048576 * 50;
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/log.txt", max_size, 5, true);
    fileSink->set_level (spdlog::level::trace);

    spdlog::logger logger("main_out", {consoleSink, fileSink});
    spdlog::register_logger (std::make_shared<spdlog::logger>(logger));
    logger.set_level (spdlog::level::debug);
    logger.enable_backtrace (32);

    try {
        logger.info("Starting Application");
        app.run ();
        logger.info ("Stopping Application");
    } catch (std::exception& e) {
        logger.critical ("Application Crashed: {}", e.what ());
        logger.dump_backtrace();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
