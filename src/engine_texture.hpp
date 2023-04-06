//
// Created by Peter Lewis on 2022-10-04.
//

#ifndef VULKANENGINE_ENGINE_TEXTURE_HPP
#define VULKANENGINE_ENGINE_TEXTURE_HPP

#include "engine_device.hpp"

namespace engine {
    class EngineTexture {
    public:
        EngineTexture(EngineDevice &device, const std::string &filepath);

        ~EngineTexture();

        VkSampler getSampler() { return sampler; };
        VkImageView getImageView() { return imageView; };
        VkImageLayout getImageLayout() { return imageLayout; };

        EngineTexture(const EngineTexture &) = delete;
        EngineTexture &operator=(const EngineTexture &) = delete;
        EngineTexture(EngineTexture &&) = delete;
        EngineTexture &operator=(EngineTexture &&) = delete;

    private:

        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

        EngineDevice &device;
        VkImage image;
        VkDeviceMemory  imageMemory;
        VkImageView imageView;
        VkSampler sampler;
        VkFormat imageFormat;
        VkImageLayout imageLayout;
    };
}


#endif //VULKANENGINE_ENGINE_TEXTURE_HPP
