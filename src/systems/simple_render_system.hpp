//
// Created by Peter Lewis on 2022-06-15.
//

#ifndef VULKANENGINE_SIMPLE_RENDER_SYSTEM_HPP
#define VULKANENGINE_SIMPLE_RENDER_SYSTEM_HPP

#include "../engine_pipeline.hpp"
#include "../engine_device.hpp"
#include "../engine_game_object.hpp"
#include "../engine_camera.hpp"
#include "../engine_frame_info.hpp"

// std
#include <memory>

namespace engine::system {
    class SimpleRenderSystem {
    public:
        SimpleRenderSystem (EngineDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        virtual ~SimpleRenderSystem ();

        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem operator=(const SimpleRenderSystem &) = delete;

        void renderGameObjects (EngineFrameInfo &frameInfo);


    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        EngineDevice &engineDevice;

        std::unique_ptr<EnginePipeline> enginePipeline;
        VkPipelineLayout pipelineLayout;
    };
} // engine::system


#endif //VULKANENGINE_SIMPLE_RENDER_SYSTEM_HPP
