//
// Created by Peter Lewis on 2022-06-15.
//

#ifndef VULKANENGINE_ENGINE_RENDERER_HPP
#define VULKANENGINE_ENGINE_RENDERER_HPP

#include "engine_window.hpp"
#include "engine_device.hpp"
#include "engine_swapchain.hpp"

// std
#include <cassert>
#include <memory>
#include <chrono>

namespace engine {
    class EngineRenderer {
    public:
        EngineRenderer (EngineWindow &window, EngineDevice &device);
        virtual ~EngineRenderer ();

        EngineRenderer(const EngineRenderer &) = delete;
        EngineRenderer operator=(const EngineRenderer &) = delete;

        VkRenderPass getSwapchainRenderpass() const { return engineSwapChain->getRenderPass(); }
        float getAspectRatio() const { return engineSwapChain->extentAspectRatio(); }
        bool isFrameInProgress() const { return isFrameStarted; }

        VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        VkCommandBuffer beginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        void recreateSwapChain();

        EngineWindow& engineWindow;
        EngineDevice& engineDevice;
        std::unique_ptr<EngineSwapChain> engineSwapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex{0};
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
} // engine

#endif //VULKANENGINE_ENGINE_RENDERER_HPP
