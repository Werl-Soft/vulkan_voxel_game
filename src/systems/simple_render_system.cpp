//
// Created by Peter Lewis on 2022-06-15.
//

#include "simple_render_system.hpp"

#include "../first_app.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include <chrono>
#include <iostream>
#include <spdlog/spdlog.h>

namespace engine::system {
    struct SimplePushConstantData {
        glm::mat4 modelMatrix {1.0f};
        glm::mat4 normalMatrix{1.0f};
    };

    SimpleRenderSystem::SimpleRenderSystem (EngineDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : engineDevice{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    SimpleRenderSystem::~SimpleRenderSystem () {
        vkDestroyPipelineLayout (engineDevice.device(), pipelineLayout, nullptr);
    }

    void SimpleRenderSystem::createPipelineLayout (VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof (SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout (engineDevice.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            spdlog::get ("vulkan")->critical ("Failed to create pipeline layout");
            throw std::runtime_error ("Failed to create pipeline layout!");
        }

    }

    void SimpleRenderSystem::createPipeline (VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        EnginePipeline::defaultPipelineConfigInfo (pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        enginePipeline = std::make_unique<EnginePipeline>(engineDevice, "assets/shaders/simple_shader.vert.spv", "assets/shaders/simple_shader.frag.spv", pipelineConfig);

    }

    void SimpleRenderSystem::renderGameObjects (EngineFrameInfo &frameInfo) {
        enginePipeline->bind (frameInfo.commandBuffer);

        vkCmdBindDescriptorSets (frameInfo.commandBuffer,
                                 VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 pipelineLayout,
                                 0,
                                 1,
                                 &frameInfo.globalDescriptorSet,
                                 0,
                                 nullptr);

        for (auto& kv : frameInfo.gameObjects) {
            auto& obj = kv.second;

            if (obj.model == nullptr) continue;

            SimplePushConstantData push {};
            push.modelMatrix = obj.transform.mat4();
            push.normalMatrix = obj.transform.normalMatrix();

            vkCmdPushConstants (
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof (SimplePushConstantData),
                    &push);
            obj.model->bind (frameInfo.commandBuffer);
            obj.model->draw (frameInfo.commandBuffer);
        }
    }
} // engine::system
