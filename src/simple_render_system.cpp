//
// Created by Peter Lewis on 2022-06-15.
//

#include "simple_render_system.hpp"

#include "first_app.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <chrono>
#include <iostream>

namespace engine {
    struct SimplePushConstantData {
        glm::mat4 transform {1.0f};
        glm::mat4 normalMatrix{1.0f};
    };

    SimpleRenderSystem::SimpleRenderSystem (EngineDevice &device, VkRenderPass renderPass) : engineDevice{device} {
        createPipelineLayout();
        createPipeline(renderPass);
    }

    SimpleRenderSystem::~SimpleRenderSystem () {
        vkDestroyPipelineLayout (engineDevice.device(), pipelineLayout, nullptr);
    }

    void SimpleRenderSystem::createPipelineLayout () {
        VkPushConstantRange pushConstantRange {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof (SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout (engineDevice.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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

    void SimpleRenderSystem::renderGameObjects (VkCommandBuffer commandBuffer, std::vector<EngineGameObject> &gameObjects, const EngineCamera &camera) {
        enginePipeline->bind (commandBuffer);

        auto projectionView = camera.getProjection () * camera.getViewMatrix ();

        for (auto& obj : gameObjects) {
            SimplePushConstantData push {};
            auto modelMatrix = obj.transform.mat4();
            push.transform = projectionView * modelMatrix;
            push.normalMatrix = obj.transform.normalMatrix();

            vkCmdPushConstants (
                    commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof (SimplePushConstantData),
                    &push);
            obj.model->bind (commandBuffer);
            obj.model->draw (commandBuffer);
        }
    }
}
