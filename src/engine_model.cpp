//
// Created by Peter Lewis on 2022-06-05.
//

#include "engine_model.hpp"
#include "engine_utils.hpp"

//libs
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <spdlog/spdlog.h>

//std
#include <cassert>
#include <unordered_map>

namespace std {
    template<>
    struct hash<engine::EngineModel::Vertex> {
        size_t operator()(engine::EngineModel::Vertex const &vertex) const {
            size_t seed = 0;
            engine::hashCombine (seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}

namespace engine {
    std::vector<VkVertexInputBindingDescription> EngineModel::Vertex::getBindingDescriptions () {
        auto bindingDescriptions = std::vector<VkVertexInputBindingDescription> (1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof (Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> EngineModel::Vertex::getAttributeDescriptions () {
        auto attributeDescriptions = std::vector<VkVertexInputAttributeDescription> {};

        attributeDescriptions.push_back ({0,0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, position))});
        attributeDescriptions.push_back ({1,0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, color))});
        attributeDescriptions.push_back ({2,0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, normal))});
        attributeDescriptions.push_back ({3,0, VK_FORMAT_R32G32_SFLOAT,    static_cast<uint32_t>(offsetof(Vertex, uv))});

        return attributeDescriptions;
    }

    EngineModel::EngineModel (EngineDevice &device, const Builder &builder): engineDevice {device} {
        createVertexBuffer (builder.vertices);
        createIndexBuffer (builder.indices);

    }

    EngineModel::~EngineModel () {
        vkDestroyBuffer(engineDevice.device(), vertexBuffer, nullptr);
        vkFreeMemory (engineDevice.device(), vertexBufferMemory, nullptr);

        if(hasIndexBuffer) {
            vkDestroyBuffer(engineDevice.device(), indexBuffer, nullptr);
            vkFreeMemory (engineDevice.device(), indexBufferMemory, nullptr);
        }
    }

    void EngineModel::bind (VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers (commandBuffer, 0, 1, buffers, offsets);

        if (hasIndexBuffer) {
            vkCmdBindIndexBuffer (commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void EngineModel::draw (VkCommandBuffer commandBuffer) {
        if (hasIndexBuffer){
            vkCmdDrawIndexed (commandBuffer, indexCount, 1, 0 ,0 ,0);
        } else {
            vkCmdDraw (commandBuffer, vertexCount, 1, 0, 0);

        }
    }

    void EngineModel::createVertexBuffer (const std::vector<Vertex> &vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount > 2 && "Vertex count must be at least 3");

        VkDeviceSize bufferSize = sizeof (vertices[0]) * vertexCount;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        engineDevice.createBuffer (
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory);
        void *data;
        vkMapMemory (engineDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy (data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory (engineDevice.device(), stagingBufferMemory);

        engineDevice.createBuffer (
                bufferSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                vertexBuffer,
                vertexBufferMemory);
        engineDevice.copyBuffer (stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(engineDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory (engineDevice.device(), stagingBufferMemory, nullptr);
    }

    void EngineModel::createIndexBuffer (const std::vector<uint32_t> &indices) {
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = indexCount > 0;
        if (!hasIndexBuffer)
            return;

        VkDeviceSize bufferSize = sizeof (indices[0]) * indexCount;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        engineDevice.createBuffer (
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory);
        void *data;
        vkMapMemory (engineDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy (data, indices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory (engineDevice.device(), stagingBufferMemory);

        engineDevice.createBuffer (
                bufferSize,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                indexBuffer,
                indexBufferMemory);

        engineDevice.copyBuffer (stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(engineDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory (engineDevice.device(), stagingBufferMemory, nullptr);
    }

    std::unique_ptr<EngineModel> EngineModel::createModelFromFile (EngineDevice &device, const std::string &filepath) {
        Builder builder{};
        builder.loadModel (filepath);
        spdlog::info ("Vertex Count: {}", builder.vertices.size());
        return std::make_unique<EngineModel>(device, builder);
    }

    void EngineModel::Builder::loadModel (const std::string &filepath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj (&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
            spdlog::error ("Failed to load \"{}\" because: {} {}", filepath, warn, err);
        }

        vertices.clear();
        indices.clear();

        std::unordered_map<Vertex, uint32_t> uniqueVerts{};
        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                Vertex vertex{};

                if (index.vertex_index >= 0) {
                    vertex.position = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.color = {
                            attrib.colors[3 * index.vertex_index + 0],
                            attrib.colors[3 * index.vertex_index + 1],
                            attrib.colors[3 * index.vertex_index + 2]
                    };
                }

                if (index.normal_index >= 0) {
                    vertex.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                    };
                }

                if (index.texcoord_index >= 0) {
                    vertex.uv = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                if(uniqueVerts.count (vertex) == 0) {
                    uniqueVerts[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back (vertex);
                }
                indices.push_back (uniqueVerts[vertex]);
            }
        }
    }
} // engine