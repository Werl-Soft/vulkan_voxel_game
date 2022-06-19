//
// Created by Peter Lewis on 2022-06-05.
//

#include "engine_model.hpp"

#include <cassert>

namespace engine {
    std::vector<VkVertexInputBindingDescription> EngineModel::Vertex::getBindingDescriptions () {
        auto bindingDescriptions = std::vector<VkVertexInputBindingDescription> (1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof (Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> EngineModel::Vertex::getAttributeDescriptions () {
        auto attributeDescriptions = std::vector<VkVertexInputAttributeDescription> (2);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }

    EngineModel::EngineModel (EngineDevice &device, const std::vector<Vertex> &vertices): engineDevice {device} {
        createVertexBuffer (vertices);

    }

    EngineModel::~EngineModel () {
        vkDestroyBuffer(engineDevice.device(), vertexBuffer, nullptr);
        vkFreeMemory (engineDevice.device(), vertexBufferMemory, nullptr);
    }

    void EngineModel::bind (VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers (commandBuffer, 0, 1, buffers, offsets);
    }

    void EngineModel::draw (VkCommandBuffer commandBuffer) {
        vkCmdDraw (commandBuffer, vertexCount, 1, 0, 0);
    }

    void EngineModel::createVertexBuffer (const std::vector<Vertex> &vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount > 2 && "Vertex count must be at least 3");

        VkDeviceSize bufferSize = sizeof (vertices[0]) * vertexCount;
        engineDevice.createBuffer (bufferSize,
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   vertexBuffer,
                                   vertexBufferMemory
                                   );
        void *data;
        vkMapMemory (engineDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy (data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory (engineDevice.device(), vertexBufferMemory);

    }

} // engine