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
            glm::vec3 position;
            glm::vec3 color;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
        };

        EngineModel (EngineDevice &device, const Builder &builder);
        virtual ~EngineModel ();

        EngineModel(const EngineModel &) = delete;
        EngineModel operator=(const EngineModel &) = delete;

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffer(const std::vector<Vertex> &vertices);
        void createIndexBuffer(const std::vector<uint32_t> &indices);

        EngineDevice &engineDevice;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        uint32_t vertexCount;

        bool hasIndexBuffer = false;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        uint32_t indexCount;
    };

} // engine

#endif //BASIC_TESTS_ENGINE_MODEL_HPP
