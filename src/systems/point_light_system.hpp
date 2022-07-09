//
// Created by Peter Lewis on 2022-07-09.
//

#ifndef VULKANENGINE_POINT_LIGHT_SYSTEM_HPP
#define VULKANENGINE_POINT_LIGHT_SYSTEM_HPP

#include "../engine_pipeline.hpp"
#include "../engine_device.hpp"
#include "../engine_game_object.hpp"
#include "../engine_camera.hpp"
#include "../engine_frame_info.hpp"

namespace engine::system {

    class PointLightSystem {
    public:
        PointLightSystem (EngineDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        virtual ~PointLightSystem();

        PointLightSystem(const PointLightSystem &) = delete;
        PointLightSystem operator=(const PointLightSystem &) = delete;

        void render (EngineFrameInfo &frameInfo);


    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        EngineDevice &engineDevice;

        std::unique_ptr<EnginePipeline> enginePipeline;
        VkPipelineLayout pipelineLayout;
    };

} // system

#endif //VULKANENGINE_POINT_LIGHT_SYSTEM_HPP
