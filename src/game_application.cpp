//
// Created by peter on 2022-01-02.
//

#include "game_application.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>

#include "vulkan_helpers.hpp"
#include "file_loader.hpp"

void game::GameApplication::run () {
    logger = spdlog::get ("main_out");
    initWindow ();
    initVulkan ();
    mainLoop ();
    cleanup ();
}

void game::GameApplication::initWindow () {
    logger->info ("Starting: GLFW Initialization");

    glfwInit ();

    logger->info("GLFW initialized");

    glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow (width, height, "Vulkan", nullptr, nullptr);
    if (window == nullptr) {
        logger->critical ("Failed to create GLFW window!");
    } else {
        logger->debug ("GLFW window created successfully");
    }
    glfwSetWindowUserPointer (window, this);
    glfwSetFramebufferSizeCallback (window, frameBufferResizeCallback);

    logger->info ("Finished: GLFW Initialization");
}

void game::GameApplication::frameBufferResizeCallback (GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<GameApplication *>(glfwGetWindowUserPointer (window));
    app->frameBufferResized = true;
}

void game::GameApplication::initVulkan () {
    logger->info ("Starting: Vulkan Initialization");

    createInstance ();
    setupDebugCallback ();
    createSurface ();
    pickPhysicalDevice ();
    createLogicalDevice ();
    createSwapChain ();
    createImageViews ();
    createRenderPass ();
    createDescriptorSetLayout ();
    createGraphicsPipeline ();
    createCommandPool ();
    createDepthResources();
    createFrameBuffers ();
    createTextureImage ();
    createTextureImageView ();
    createTextureSampler ();
    createVertexBuffer ();
    createIndexBuffer ();
    createUniformBuffers ();
    createDescriptorPool ();
    createDescriptorSets ();
    createCommandBuffers ();
    createSyncObjects ();

    logger->info ("Finished: Vulkan Initialization");
}

void game::GameApplication::createInstance () {
    logger->info ("Starting: Vulkan Instance Creation");

    // Check if Validation Layers are requested, and they are supported on current system
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
            0,
            nullptr, // enabled layers
            static_cast<uint32_t>(extensions.size ()),
            extensions.data ()); // enabled extensions

    if (vk_helpers::enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size ());
        createInfo.ppEnabledLayerNames = validationLayers.data ();
    }

    // Try to create the actual instance
    try {
        instance = vk::createInstanceUnique (createInfo, nullptr);
    } catch (vk::SystemError &err) {
        logger->critical ("Failed to create instance: {}", err.what());
        throw std::runtime_error ("Failed to create instance");
    }


    //TODO Remove Later
    logger->info ("Available Extensions:");

    for (const auto &extension: vk::enumerateInstanceExtensionProperties ()) {
        logger->info ("\t{}", extension.extensionName);
    }

    dld = vk::DispatchLoaderDynamic (instance->operator VkInstance_T * (), vkGetInstanceProcAddr);

    logger->info ("Finished: Vulkan Instance Creation");
}

void game::GameApplication::setupDebugCallback () {
    if (! vk_helpers::enableValidationLayers)
        return;

    logger->info ("Starting: Debug Callback Creation");

    auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT (
            vk::DebugUtilsMessengerCreateFlagsEXT (),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debugCallback,
            nullptr);

    instance->createDebugUtilsMessengerEXTUnique (createInfo, nullptr, dld);

    logger->info ("Finished: Debug Callback Creation");
}

VkBool32 game::GameApplication::debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                               VkDebugUtilsMessageTypeFlagsEXT messageType,
                                               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                               void *pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void game::GameApplication::createSurface () {
    logger->info ("Starting: Vulkan Surface Creation");

    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface (instance->operator VkInstance_T * (), window, nullptr, &rawSurface) != VK_SUCCESS) {
        logger->critical ("Failed to create window surface!");
        throw std::runtime_error ("Failed to create window surface!");
    }
    surface = static_cast<vk::SurfaceKHR>(rawSurface);

    logger->info ("Finished: Vulkan Surface Creation");
}

void game::GameApplication::pickPhysicalDevice () {
    logger->info ("Starting: Choose Physical Device");
    auto devices = instance->enumeratePhysicalDevices ();
    if (devices.empty ()) {
        logger->critical ("Failed to find GPUs with Vulkan support!");
        throw std::runtime_error ("Failed to find GPUs with Vulkan support!");
    }

    logger->info ("Available Devices: {}", devices.size());
    int n = 0;
    for (const auto &d: devices) {
        logger->info ("\t{}: {}", n, d.getProperties ().deviceName);
        n++;
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
    } else {
        // TODO Remove Later
        logger->info ("Device Chosen: {}", physicalDevice.getProperties ().deviceName);
        auto supportedExt = physicalDevice.enumerateDeviceExtensionProperties ();
        for (auto ext : supportedExt) {
            logger->info ("\t{}", ext.extensionName);
        }
    }

    logger->info ("Finished: Choose Physical Device: {}", physicalDevice.getProperties ().deviceName);
}

void game::GameApplication::createLogicalDevice () {
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
    deviceFeatures.samplerAnisotropy = true;

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

void game::GameApplication::createSwapChain () {
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

    swapChainImages = device->getSwapchainImagesKHR(swapChain);

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    logger->info ("Finished: Swap Chain Creation");
}

void game::GameApplication::createImageViews () {
    logger->info ("Starting: Image View Creation");

    swapChainImageViews.resize (swapChainImages.size ());

    for (size_t i = 0; i < swapChainImages.size (); i ++) {
        try {
            swapChainImageViews[i] = vk_helpers::createImageView (swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor, device);
        }
        catch (vk::SystemError &err) {
            logger->critical ("Failed to create image view {}: {}", i, err.what());
            throw std::runtime_error ("failed to create image views!");
        }
    }
    logger->info ("Finished: Image View Creation");
}

void game::GameApplication::createRenderPass () {
    logger->info ("Starting: Render Pass Creation");

    vk::AttachmentDescription depthAttachment = {};
    depthAttachment.format = vk_helpers::findDepthFormat (physicalDevice);
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachRef = {};
    depthAttachRef.attachment = 1;
    depthAttachRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

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

    vk::SubpassDescription subPass{};
    subPass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colorAttachmentRef;
    subPass.pDepthStencilAttachment = &depthAttachRef;

    vk::SubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subPass;
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

void game::GameApplication::createDescriptorSetLayout () {
    logger->info ("Starting: Descriptor Set Layout Creation");

    vk::DescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

    vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    try {
        descriptorSetLayout = device->createDescriptorSetLayout (layoutInfo);
    } catch (vk::SystemError &err) {
        logger->critical ("Failed to create descriptor set layout: {}", err.what());
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    logger->info ("Finished: Descriptor Set Layout Creation");
}

void game::GameApplication::createGraphicsPipeline () {
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

    vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.depthTestEnable = true;
    depthStencil.depthWriteEnable = true;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    // used for culling
    depthStencil.depthBoundsTestEnable = false;
    // depth stencil operations
    depthStencil.stencilTestEnable = false;

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
    pipelineInfo.pDepthStencilState = &depthStencil;

    try {
        graphicsPipeline = device->createGraphicsPipeline (nullptr, pipelineInfo).value;
    }
    catch (vk::SystemError &err) {
        logger->critical ("Failed to create graphics pipeline: {}", err.what());
        throw std::runtime_error ("failed to create graphics pipeline!");
    }
    logger->info ("Finished: Graphics Pipeline Creation");
}

void game::GameApplication::createFrameBuffers () {
    logger->info ("Starting: Frame Buffer Creation");

    swapChainFrameBuffers.resize (swapChainImageViews.size ());

    for (size_t i = 0; i < swapChainImageViews.size (); i ++) {
        std::array<vk::ImageView, 2> attachments = {
                swapChainImageViews[i].get(),
                depthImageView.get()
        };

        vk::FramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.renderPass = renderPass;
        frameBufferCreateInfo.attachmentCount = attachments.size();
        frameBufferCreateInfo.pAttachments = attachments.data();
        frameBufferCreateInfo.width = swapChainExtent.width;
        frameBufferCreateInfo.height = swapChainExtent.height;
        frameBufferCreateInfo.layers = 1;

        try {
            swapChainFrameBuffers[i] = device->createFramebuffer (frameBufferCreateInfo);
        } catch (vk::SystemError &err) {
            logger->critical ("Failed to create frame buffer: {}", err.what());
            throw std::runtime_error ("Failed to create frame buffer");
        }
    }

    logger->info ("Finished: Frame Buffer Creation");
}

void game::GameApplication::createCommandPool () {
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


void game::GameApplication::createDepthResources () {
    logger->info ("Starting: Depth Resource Creation");

    vk::Format depthFormat = vk_helpers::findDepthFormat (physicalDevice);

    depthImage = vk_helpers::createImage (swapChainExtent.width,
                                          swapChainExtent.height,
                                          depthFormat,
                                          vk::ImageTiling::eOptimal,
                                          vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                                          depthImageMemory,
                                          device,
                                          physicalDevice);

depthImageView = vk_helpers::createImageView(depthImage.get(), depthFormat, vk::ImageAspectFlagBits::eDepth, device);

    logger->info ("Finished: Depth Resource Creation");
}


void game::GameApplication::createTextureImage () {
    logger->info ("Starting: Texture Image Creation");

    auto textureData = file_loader::readFileImage("assets/textures/statue.jpg");

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingDeviceMemory;

    vk_helpers::createBuffer (textureData.getSize(),
                              vk::BufferUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                              stagingBuffer,
                              stagingDeviceMemory,
                              device,
                              physicalDevice);

    void* data = device->mapMemory (stagingDeviceMemory, 0 , static_cast<size_t>(textureData.getSize()));
    {
        memcpy(data, textureData.data, static_cast<size_t>(textureData.getSize()));
    }
    device->unmapMemory (stagingDeviceMemory);

    file_loader::freeImageData (textureData);

    textureImage = vk_helpers::createImage (textureData.width,
                             textureData.height,
                             vk::Format::eR8G8B8A8Srgb,
                             vk::ImageTiling::eOptimal,
                             vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                             vk::MemoryPropertyFlagBits::eDeviceLocal,
                             textureMemory,
                             device,
                             physicalDevice);

    vk_helpers::transitionImageLayout (textureImage.get(), vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, commandPool, device, graphicsQueue);
    vk_helpers::copyBufferToImage (stagingBuffer, textureImage.get(), textureData.width, textureData.height, commandPool, device, graphicsQueue);
    vk_helpers::transitionImageLayout (textureImage.get(), vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, commandPool, device, graphicsQueue);

    device->destroyBuffer (stagingBuffer);
    device->freeMemory (stagingDeviceMemory);

    logger->info ("Finished: Texture Image Creation");
}

void game::GameApplication::createTextureImageView () {
    textureImageView = vk_helpers::createImageView (textureImage.get(), vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, device);
}


void game::GameApplication::createTextureSampler () {
    vk::SamplerCreateInfo samplerInfo = {};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

    auto properties = physicalDevice.getProperties ();

    samplerInfo.anisotropyEnable = true;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = false;
    samplerInfo.compareEnable = false;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    try {
        textureSampler = device->createSamplerUnique (samplerInfo);
    } catch (vk::SystemError &err) {
        logger->critical ("Failed to create texture sampler: {}", err.what());
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

void game::GameApplication::createVertexBuffer () {
    logger->info ("Starting: Vertex Buffer Creation");

    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    vk_helpers::createBuffer (bufferSize,
                              vk::BufferUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                              stagingBuffer,
                              stagingBufferMemory,
                              device,
                              physicalDevice);

    // Copy data to GPU
    void* data = device->mapMemory (stagingBufferMemory, 0, bufferSize);
    {
        memcpy (data, vertices.data (), bufferSize);
    }
    device->unmapMemory (stagingBufferMemory);

    vk_helpers::createBuffer (bufferSize,
                              vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                              vertexBuffer,
                              vertexBufferMemory,
                              device,
                              physicalDevice);

    vk_helpers::copyBuffer (stagingBuffer, vertexBuffer, bufferSize, commandPool, device, graphicsQueue);

    device->destroyBuffer (stagingBuffer);
    device->freeMemory (stagingBufferMemory);

    logger->info ("Finished: Vertex Buffer Creation");
}

void game::GameApplication::createIndexBuffer () {
    logger->info ("Starting: Index Buffer Creation");

    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    vk_helpers::createBuffer (bufferSize,
                              vk::BufferUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                              stagingBuffer,
                              stagingBufferMemory,
                              device,
                              physicalDevice);

    // Copy data to GPU
    void* data = device->mapMemory (stagingBufferMemory, 0, bufferSize);
    {
        memcpy (data, indices.data (), bufferSize);
    }
    device->unmapMemory (stagingBufferMemory);

    vk_helpers::createBuffer (bufferSize,
                              vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                              indexBuffer,
                              indexBufferMemory,
                              device,
                              physicalDevice);

    vk_helpers::copyBuffer (stagingBuffer, indexBuffer, bufferSize, commandPool, device, graphicsQueue);

    device->destroyBuffer (stagingBuffer);
    device->freeMemory (stagingBufferMemory);

    logger->info ("Finished: Index Buffer Creation");
}

void game::GameApplication::createUniformBuffers () {
    logger->info ("Starting: Uniform Buffer Creation");

    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapChainImages.size());
    uniformBufferMemory.resize (swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vk_helpers::createBuffer (bufferSize,
                                  vk::BufferUsageFlagBits::eUniformBuffer,
                                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                  uniformBuffers[i],
                                  uniformBufferMemory[i],
                                  device,
                                  physicalDevice);
    }

    logger->info ("Finished: Uniform Buffer Creation");
}

void game::GameApplication::createDescriptorPool () {
    logger->info ("Starting: Descriptor Pool Creation");

    std::array<vk::DescriptorPoolSize, 2> poolSize = {};
    poolSize[0].type = vk::DescriptorType::eUniformBuffer;
    poolSize[0].descriptorCount = swapChainImages.size();

    poolSize[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSize[1].descriptorCount = swapChainImages.size();


    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.poolSizeCount = poolSize.size();
    poolInfo.pPoolSizes = poolSize.data();
    poolInfo.maxSets = swapChainImages.size();

    try{
        descriptorPool = device->createDescriptorPool (poolInfo);
    } catch (vk::SystemError &err) {
        logger->critical ("Failed to create descriptor pool: {}", err.what());
        throw std::runtime_error("Failed to create descriptor pool");
    }

    logger->info ("Finished: Descriptor Pool Creation");
}

void game::GameApplication::createDescriptorSets () {
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

        vk::DescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = textureImageView.get();
        imageInfo.sampler = textureSampler.get();

        std::array<vk::WriteDescriptorSet, 2> descriptorWriter = {};
        descriptorWriter[0].dstSet = descriptorSets[i];
        descriptorWriter[0].dstBinding = 0;
        descriptorWriter[0].dstArrayElement = 0;
        descriptorWriter[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWriter[0].descriptorCount = 1;
        descriptorWriter[0].pBufferInfo = &bufferInfo;

        descriptorWriter[1].dstSet = descriptorSets[i];
        descriptorWriter[1].dstBinding = 1;
        descriptorWriter[1].dstArrayElement = 0;
        descriptorWriter[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWriter[1].descriptorCount = 1;
        descriptorWriter[1].pImageInfo = &imageInfo;

        device->updateDescriptorSets (descriptorWriter.size(), descriptorWriter.data(), 0, nullptr);
    }

    logger->info ("Finished: Descriptor Set Creation");
}

void game::GameApplication::createCommandBuffers () {
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

    /**
     * Tries to set up command buffer
     * Sets up render pass info
     * Binds data from render pass info to command buffers
     */
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

        std::array<vk::ClearValue, 2> clearValues;
        clearValues[0] = {std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1] = vk::ClearValue({1.0f, 0});
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffers[i].beginRenderPass (renderPassInfo, vk::SubpassContents::eInline);
        {
            commandBuffers[i].bindPipeline (vk::PipelineBindPoint::eGraphics, graphicsPipeline);
            vk::Buffer vertexBuffers[] = {vertexBuffer};
            vk::DeviceSize offsets[] = {0};
            commandBuffers[i].bindVertexBuffers (0, 1, vertexBuffers, offsets);
            commandBuffers[i].bindIndexBuffer (indexBuffer, 0, vk::IndexType::eUint16);
            commandBuffers[i].bindDescriptorSets (vk::PipelineBindPoint::eGraphics,
                                                  pipelineLayout,
                                                  0,
                                                  1,
                                                  &descriptorSets[i],
                                                  0,
                                                  nullptr);
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

void game::GameApplication::createSyncObjects () {
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

void game::GameApplication::mainLoop () {
    logger->info ("Starting: Main Loop");
    while (! glfwWindowShouldClose (window)) {
        glfwPollEvents ();
        drawFrame ();
    }

    device->waitIdle ();
    logger->info ("Finished: Main Loop");
}

void game::GameApplication::drawFrame () {
    device->waitForFences (inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max ());
    device->resetFences (inFlightFences[currentFrame]);

    // Checks to see if swap chain needs to be recreated
    uint32_t imageIndex;
    try {
        auto result = device->acquireNextImageKHR (swapChain,
                                                              std::numeric_limits<uint64_t>::max (),
                                                              imageAvailableSemaphores[currentFrame],
                                                              nullptr);
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

    vk::SwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
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

void game::GameApplication::updateUniformBuffer (uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period> (currentTime - startTime).count();

    UniformBufferObject ubo = {};
    // Rotation
    ubo.model = glm::rotate (glm::mat4(1.0f), time * glm::radians (90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // Translation
    ubo.view = glm::lookAt (glm::vec3(2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // Perspective
    ubo.proj = glm::perspective (glm::radians (45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    // Flip Y co-ord because GLM was created for OpenGL that uses the opposite Y co-ords
    ubo.proj[1][1] *= -1;

    void* data = device->mapMemory (uniformBufferMemory[currentImage], 0, sizeof(ubo));
    {
        memcpy (data, &ubo, sizeof (ubo));
    }
    device->unmapMemory (uniformBufferMemory[currentImage]);
}

void game::GameApplication::recreateSwapChain () {
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
    createDepthResources ();
    createFrameBuffers ();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers ();

    logger->info ("Finished: Swap Chain Recreation");
}

void game::GameApplication::cleanupSwapChain () {
    logger->info ("Starting: Swap Chain Cleanup");

    for (auto frameBuffer: swapChainFrameBuffers) {
        device->destroyFramebuffer (frameBuffer);
    }

    device->freeCommandBuffers (commandPool, commandBuffers);

    device->destroyPipeline (graphicsPipeline);
    device->destroyRenderPass (renderPass);
    device->destroyPipelineLayout (pipelineLayout);

    device->destroySwapchainKHR (swapChain);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        device->destroyBuffer (uniformBuffers[i]);
        device->freeMemory (uniformBufferMemory[i]);
    }

    device->destroyDescriptorPool (descriptorPool);

    logger->info ("Finished: Swap Chain Cleanup");
}

void game::GameApplication::cleanup () {
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
