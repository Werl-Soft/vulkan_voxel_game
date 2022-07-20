//
// Created by Peter Lewis on 2022-07-08.
//

#ifndef VULKANENGINE_ENGINE_FRAME_INFO_HPP
#define VULKANENGINE_ENGINE_FRAME_INFO_HPP

#include "engine_camera.hpp"
#include "engine_game_object.hpp"

// lib
#include <vulkan/vulkan.h>

#define MAX_LIGHTS 10

namespace engine {

    struct PointLight {
        glm::vec4 position{}; // ignore w
        glm::vec4 color{}; // w is intensity
    };

    struct GlobalUBO {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
        glm::mat4 inverseView{1.0f};
        glm::vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.05f}; // w is light intensity
        PointLight pointLights[MAX_LIGHTS];
        int numLights;
    };

    struct EngineFrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        EngineCamera &camera;
        VkDescriptorSet globalDescriptorSet;
        EngineGameObject::Map &gameObjects;
    };

} // engine

#endif //VULKANENGINE_ENGINE_FRAME_INFO_HPP
