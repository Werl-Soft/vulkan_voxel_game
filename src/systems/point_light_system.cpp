//
// Created by Peter Lewis on 2022-07-09.
//

#include "point_light_system.hpp"

#include "../first_app.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include <chrono>
#include <iostream>

namespace engine::system {
    struct SimplePushConstantData {
        glm::mat4 modelMatrix {1.0f};
        glm::mat4 normalMatrix{1.0f};
    };

    PointLightSystem::PointLightSystem (EngineDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : engineDevice{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    PointLightSystem::~PointLightSystem () {
        vkDestroyPipelineLayout (engineDevice.device(), pipelineLayout, nullptr);
    }

    void PointLightSystem::createPipelineLayout (VkDescriptorSetLayout globalSetLayout) {
//        VkPushConstantRange pushConstantRange {};
//        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
//        pushConstantRange.offset = 0;
//        pushConstantRange.size = sizeof (SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout (engineDevice.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error ("Failed to create pipeline layout!");
        }

    }

    void PointLightSystem::createPipeline (VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        EnginePipeline::defaultPipelineConfigInfo (pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        enginePipeline = std::make_unique<EnginePipeline>(
                engineDevice,
                "assets/shaders/point_light.vert.spv",
                "assets/shaders/point_light.frag.spv",
                pipelineConfig);

    }

    void PointLightSystem::render (EngineFrameInfo &frameInfo) {
        enginePipeline->bind (frameInfo.commandBuffer);

        vkCmdBindDescriptorSets (frameInfo.commandBuffer,
                                 VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 pipelineLayout,
                                 0,
                                 1,
                                 &frameInfo.globalDescriptorSet,
                                 0,
                                 nullptr);

        vkCmdDraw (frameInfo.commandBuffer, 6, 1, 0, 0);
    }
} // system