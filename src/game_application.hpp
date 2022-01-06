//
// Created by peter on 2022-01-02.
//

#ifndef BASIC_TESTS_GAME_APPLICATION_HPP
#define BASIC_TESTS_GAME_APPLICATION_HPP

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

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

    class GameApplication {

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

        void run ();

    private:
        void initWindow ();

        static void frameBufferResizeCallback (GLFWwindow *window, int width, int height);

        void initVulkan ();

        /****************
         * Vulkan Stuff *
         ****************/
        /**
         * Creates the Vulkan Instance
         */
        void createInstance ();

        void setupDebugCallback ();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                             void *pUserData);

        void createSurface ();

        void pickPhysicalDevice ();

        void createLogicalDevice ();

        void createSwapChain ();

        void createImageViews ();

        void createRenderPass ();

        void createDescriptorSetLayout();

        void createGraphicsPipeline ();

        void createFrameBuffers ();

        void createCommandPool ();

        void createVertexBuffer ();

        void createIndexBuffer();

        void createUniformBuffers();

        void createDescriptorPool();

        void createDescriptorSets();

        void createCommandBuffers ();

        void createSyncObjects ();

        void mainLoop ();

        void drawFrame ();

        /**
         * Moves camera to the specified view
         * @param currentImage: the current image view
         */
        void updateUniformBuffer(uint32_t currentImage);

        void recreateSwapChain ();

        void cleanupSwapChain ();

        void cleanup ();
    };
}


#endif //BASIC_TESTS_GAME_APPLICATION_HPP
