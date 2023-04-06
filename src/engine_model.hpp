//
// Created by Peter Lewis on 2022-06-05.
//

#ifndef BASIC_TESTS_ENGINE_MODEL_HPP
#define BASIC_TESTS_ENGINE_MODEL_HPP

#include "engine_device.hpp"
#include "engine_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

//std
#include <memory>

namespace engine {

    class EngineModel {
    public:

        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
            glm::vec2 uv{};

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex &other) const {
                return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
            }
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};

            void loadModel(const std::string &filepath);
            void loadNoise(int sizeX, int sizeY);
            void loadNoise(int sizeX, int sizeY, const std::string& encodedNoise);
        };

        EngineModel (EngineDevice &device, const Builder &builder);
        virtual ~EngineModel ();

        EngineModel(const EngineModel &) = delete;
        EngineModel operator=(const EngineModel &) = delete;

        static std::unique_ptr<EngineModel> createModelFromFile (EngineDevice &device, const std::string &filepath);
        static std::unique_ptr<EngineModel> createModelFromNoise (EngineDevice &device, int xSize, int zSize);

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffer(const std::vector<Vertex> &vertices);
        void createIndexBuffer(const std::vector<uint32_t> &indices);

        EngineDevice &engineDevice;

        std::unique_ptr<EngineBuffer> vertexBuffer;
        uint32_t vertexCount;

        bool hasIndexBuffer = false;
        std::unique_ptr<EngineBuffer> indexBuffer;
        uint32_t indexCount;
    };

} // engine

#endif //BASIC_TESTS_ENGINE_MODEL_HPP
