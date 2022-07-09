//
// Created by Peter Lewis on 2022-07-08.
//

#ifndef VULKANENGINE_ENGINE_FRAME_INFO_HPP
#define VULKANENGINE_ENGINE_FRAME_INFO_HPP

#include "engine_camera.hpp"
#include "engine_game_object.hpp"

// lib
#include <vulkan/vulkan.h>

namespace engine {

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
