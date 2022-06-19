//
// Created by Peter Lewis on 2022-05-18.
//

#ifndef BASIC_TESTS_ENGINE_PIPELINE_HPP
#define BASIC_TESTS_ENGINE_PIPELINE_HPP

#include "engine_device.hpp"

#include <string>
#include <vector>

namespace engine {
    struct PipelineConfigInfo {
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {};
        VkPipelineRasterizationStateCreateInfo rasterizationInfo {};
        VkPipelineMultisampleStateCreateInfo multisampleInfo {};
        VkPipelineColorBlendAttachmentState colorBlendAttachment {};
        VkPipelineColorBlendStateCreateInfo colorBlendInfo {};
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo {};
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };

    class EnginePipeline {
    public:
        EnginePipeline (EngineDevice &device, const std::string &vertexFilepath, const std::string &fragFilepath, const PipelineConfigInfo &configInfo);
        ~EnginePipeline();

        EnginePipeline(const EnginePipeline &) = delete;
        EnginePipeline &operator=(const EnginePipeline &) = delete;

        void bind(VkCommandBuffer commandBuffer);
        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

    private:
        static std::vector<char> readFile(const std::string& filepath);

        void createGraphicsPipeline (const std::string &vertexFilepath, const std::string &fragFilepath, const PipelineConfigInfo &configInfo);

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

        EngineDevice &engineDevice;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };
}

#endif //BASIC_TESTS_ENGINE_PIPELINE_HPP
