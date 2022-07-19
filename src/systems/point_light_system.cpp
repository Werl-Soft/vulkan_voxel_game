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

    struct PointLightPushConstants {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius;
    };

    PointLightSystem::PointLightSystem (EngineDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : engineDevice{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    PointLightSystem::~PointLightSystem () {
        vkDestroyPipelineLayout (engineDevice.device(), pipelineLayout, nullptr);
    }

    void PointLightSystem::createPipelineLayout (VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof (PointLightPushConstants);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout (engineDevice.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error ("Failed to create pipeline layout!");
        }

    }

    void PointLightSystem::createPipeline (VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        EnginePipeline::defaultPipelineConfigInfo (pipelineConfig);
        pipelineConfig.attributeDiscriptions.clear();
        pipelineConfig.bindingDescriptions.clear();
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

        for (auto &kv: frameInfo.gameObjects) {
            auto &obj = kv.second;
            if (obj.pointLight == nullptr)
                continue;

            PointLightPushConstants push{};
            push.position = glm::vec4(obj.transform.translation, 1.0f);
            push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
            push.radius = obj.transform.scale.x;

            vkCmdPushConstants (
                    frameInfo.commandBuffer,
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof (PointLightPushConstants),
                    &push);

            vkCmdDraw (frameInfo.commandBuffer, 6, 1, 0, 0);
        }

    }

    void PointLightSystem::update (EngineFrameInfo &frameInfo, GlobalUBO &ubo) {
        auto rotateLight = glm::rotate (glm::mat4(1.0f), frameInfo.frameTime, {0.0f, -1.0f, 0.0f});
        int lightIndex = 0;
        for (auto& kv : frameInfo.gameObjects) {
            auto& obj = kv.second;
            if (obj.pointLight == nullptr)
                continue;
            // update light position
            obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.0f));

            // copy light to UBO
            ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.0f);
            ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);

            lightIndex += 1;
        }
        ubo.numLights = lightIndex;
    }
} // engine::system