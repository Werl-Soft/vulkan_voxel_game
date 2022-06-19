//
// Created by Peter Lewis on 2022-06-05.
//

#ifndef BASIC_TESTS_ENGINE_MODEL_HPP
#define BASIC_TESTS_ENGINE_MODEL_HPP

#include "engine_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

    class EngineModel {
    public:

        struct Vertex {
            glm::vec2 position;
            glm::vec3 color;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };

        EngineModel (EngineDevice &device, const std::vector<Vertex> &vertices);
        virtual ~EngineModel ();

        EngineModel(const EngineModel &) = delete;
        EngineModel operator=(const EngineModel &) = delete;

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffer(const std::vector<Vertex> &vertices);

        EngineDevice &engineDevice;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        uint32_t vertexCount;
    };

} // engine

#endif //BASIC_TESTS_ENGINE_MODEL_HPP
