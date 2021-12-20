#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>
#include <set>
#include <fstream>

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

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
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
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;

    std::vector<vk::Framebuffer> swapChainFrameBuffers;

    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer, std::allocator<vk::CommandBuffer>> commandBuffers;

    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
    size_t currentFrame = 0;

public:
    bool frameBufferResized = false;

private:
    void initWindow() {
        glfwInit();

        glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow (width, height, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer (window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->frameBufferResized = true;
    }

    void initVulkan() {
        createInstance();
        setupDebugCallback();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFrameBuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop() {
        while(! glfwWindowShouldClose (window)) {
            glfwPollEvents();
            drawFrame();
        }

        device->waitIdle ();
    }

    void cleanupSwapChain() {
        for (auto frameBuffer : swapChainFrameBuffers) {
            device->destroyFramebuffer (frameBuffer);
        }

        device->freeCommandBuffers (commandPool, commandBuffers);

        device->destroyPipeline (graphicsPipeline);
        device->destroyRenderPass (renderPass);
        device->destroyPipelineLayout (pipelineLayout);

        for (auto imageView : swapChainImageViews) {
            device->destroyImageView (imageView);
        }
        device->destroySwapchainKHR (swapChain);
    }

    void cleanup() {
        // NOTE: instance destruction is handled by UniqueInstance, same for device

        cleanupSwapChain();

        for (size_t i = 0; i < maxFramesInFlight; i++) {
            device->destroySemaphore (renderFinishedSemaphores[i]);
            device->destroySemaphore (imageAvailableSemaphores[i]);
            device->destroyFence (inFlightFences[i]);
        }

        device->destroyCommandPool (commandPool);

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
    void recreateSwapChain() {
        width = 0;
        height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize (window, &width, &height);
            glfwWaitEvents();
        }

        device->waitIdle ();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFrameBuffers();
        createCommandBuffers();
    }

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
        }catch (vk::SystemError &err) {
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
        if (devices.empty()) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        for (const auto& d : devices) {
            if (isDeviceSuitable(d)) {
                physicalDevice = d;
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

        queueCreateInfos.reserve(uniqueQueueFamilies.size());
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            queueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), queueFamily,1, &queuePriority);
        }

        auto deviceFeatures = vk::PhysicalDeviceFeatures();
        auto createInfo = vk::DeviceCreateInfo(
                vk::DeviceCreateFlags(),
                static_cast<uint32_t>(queueCreateInfos.size()),
                queueCreateInfos.data());
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        try {
            device = physicalDevice.createDeviceUnique(createInfo);
        } catch (vk::SystemError &err) {
            throw std::runtime_error("failed to create logical device!");
        }

        graphicsQueue = device->getQueue(indices.graphicsFamily.value(), 0);
        presentQueue = device->getQueue (indices.presentFamily.value(), 0);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo(
                vk::SwapchainCreateFlagsKHR(),
                surface,
                imageCount,
                surfaceFormat.format,
                surfaceFormat.colorSpace,
                extent,
                1, // imageArrayLayers
                vk::ImageUsageFlagBits::eColorAttachment);

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

        createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

        try {
            swapChain = device->createSwapchainKHR(createInfo);
        }
        catch (vk::SystemError &err) {
            throw std::runtime_error("failed to create swap chain!");
        }

        swapChainImages = device->getSwapchainImagesKHR(swapChain);

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize (swapChainImages.size());

        for (size_t i = 0; i <swapChainImages.size(); i++) {
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
                swapChainImageViews[i] = device->createImageView(createInfo);
            }
            catch (vk::SystemError &err) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    void createRenderPass() {
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
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFiles ("assets/shaders/vert.spv");
        auto fragShaderCode = readFiles ("assets/shaders/frag.spv");

        auto vertShaderModule = createShaderModule (vertShaderCode);
        auto fragShaderModule = createShaderModule (fragShaderCode);

        vk::PipelineShaderStageCreateInfo shaderStages[] = {
                {
                        vk::PipelineShaderStageCreateFlags(),
                        vk::ShaderStageFlagBits::eVertex,
                        *vertShaderModule,
                        "main"
                },
                {
                        vk::PipelineShaderStageCreateFlags(),
                        vk::ShaderStageFlagBits::eFragment,
                        *fragShaderModule,
                        "main"
                }
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        vk::Viewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D(0, 0);
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
        rasterizer.frontFace = vk::FrontFace::eClockwise;
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
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        try {
            pipelineLayout = device->createPipelineLayout(pipelineLayoutInfo);
        } catch (vk::SystemError &err) {
            throw std::runtime_error("failed to create pipeline layout!");
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
            graphicsPipeline = device->createGraphicsPipeline(nullptr, pipelineInfo).value;
        }
        catch (vk::SystemError &err) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }

    void createFrameBuffers() {
        swapChainFrameBuffers.resize (swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
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
                throw std::runtime_error("failed to create frame buffer");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies (physicalDevice);

        vk::CommandPoolCreateInfo poolInfo = {};
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        try {
            commandPool = device->createCommandPool (poolInfo);
        } catch (vk::SystemError &err) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize (swapChainFrameBuffers.size());

        vk::CommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = vk::CommandBufferLevel::ePrimary;
        allocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        try {
            commandBuffers = device->allocateCommandBuffers (allocateInfo);
        } catch (vk::SystemError &err) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            vk::CommandBufferBeginInfo beginInfo = {};
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

            try {
                commandBuffers[i].begin (beginInfo);
            } catch (vk::SystemError &err) {
                throw std::runtime_error("Failed to begin recording command buffer!");
            }

            vk::RenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFrameBuffers[i];
            renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
            renderPassInfo.renderArea.extent = swapChainExtent;

            vk::ClearValue clearColor = {std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            commandBuffers[i].beginRenderPass (renderPassInfo, vk::SubpassContents::eInline);
            commandBuffers[i].bindPipeline (vk::PipelineBindPoint::eGraphics, graphicsPipeline);
            commandBuffers[i].draw (3,1,0,0);
            commandBuffers[i].endRenderPass ();

            try {
                commandBuffers[i].end ();
            } catch (vk::SystemError &err) {
                throw std::runtime_error("Failed to record command buffer!");
            }
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize (maxFramesInFlight);
        renderFinishedSemaphores.resize (maxFramesInFlight);
        inFlightFences.resize (maxFramesInFlight);

        try {
            for (size_t i = 0; i < maxFramesInFlight; i++) {
                imageAvailableSemaphores[i] = device->createSemaphore ({});
                renderFinishedSemaphores[i] = device->createSemaphore ({});
                inFlightFences[i] = device->createFence ({vk::FenceCreateFlagBits::eSignaled});
            }
        } catch (vk::SystemError &err) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }

    void drawFrame(){
        device->waitForFences (1,&inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
        device->resetFences (1, &inFlightFences[currentFrame]);

        uint32_t imageIndex;
        try {
            vk::ResultValue result = device->acquireNextImageKHR (swapChain, std::numeric_limits<uint64_t>::max(),
                                                                  imageAvailableSemaphores[currentFrame], nullptr);
            imageIndex = result.value;
        } catch (vk::OutOfDateKHRError &err) {
            recreateSwapChain();
            return;
        } catch (vk::SystemError &err) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        vk::SubmitInfo submitInfo = {};
        vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame] };
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        device->resetFences (1, &inFlightFences[currentFrame]);

        try {
            graphicsQueue.submit (submitInfo, inFlightFences[currentFrame]);
        } catch (vk::SystemError &err) {
            throw std::runtime_error("Failed to submit draw command buffer!");
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
            resultPresent = presentQueue.presentKHR(presentInfo);
        } catch (vk::OutOfDateKHRError &err) {
            resultPresent = vk::Result::eErrorOutOfDateKHR;
        } catch (vk::SystemError &err) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        if (resultPresent == vk::Result::eSuboptimalKHR || frameBufferResized) {
            std::cout << "swap chain out of date/suboptimal/window resized - recreating" << std::endl;
            frameBufferResized = false;
            recreateSwapChain();
            return;
        }

        currentFrame = (currentFrame + 1) % maxFramesInFlight;
    }

    // Helper Functions

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
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

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes) {
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

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            glfwGetFramebufferSize (window, &width, &height);

            vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device) {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);

        return details;
    }

    bool isDeviceSuitable(vk::PhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport (device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : device.enumerateDeviceExtensionProperties ()) {
            requiredExtensions.erase (extension.extensionName);
        }
        return requiredExtensions.empty();
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

    static std::vector<char> readFiles(const std::string& filename) {
        std::ifstream file (filename, std::ios::ate | std::ios::binary);

        if(!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        std::streamsize fileSize = (std::streamsize)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg (0);
        file.read (buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    vk::UniqueShaderModule createShaderModule (const std::vector<char>& code) {
        try {
            return device->createShaderModuleUnique ({vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t*>(code.data())});
        } catch (vk::SystemError& error) {
            throw std::runtime_error ("failed to create shader module!");
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};

int main(int argc, char* argv[]) {
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
