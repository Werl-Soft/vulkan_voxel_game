//
// Created by Peter Lewis on 2022-06-15.
//

#ifndef VULKANENGINE_SIMPLE_RENDER_SYSTEM_HPP
#define VULKANENGINE_SIMPLE_RENDER_SYSTEM_HPP

#include "engine_pipeline.hpp"
#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "engine_camera.hpp"

// std
#include <memory>

namespace engine {
    class SimpleRenderSystem {
    public:
        SimpleRenderSystem (EngineDevice &device, VkRenderPass renderPass);
        virtual ~SimpleRenderSystem ();

        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem operator=(const SimpleRenderSystem &) = delete;

        void renderGameObjects (VkCommandBuffer commandBuffer, std::vector<EngineGameObject> &gameObjects, const EngineCamera &camera);


    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        EngineDevice &engineDevice;

        std::unique_ptr<EnginePipeline> enginePipeline;
        VkPipelineLayout pipelineLayout;
    };
}


#endif //VULKANENGINE_SIMPLE_RENDER_SYSTEM_HPP
