//
// Created by peter on 2021-12-20.
//

#include "file_loader.hpp"

#include <spdlog/spdlog.h>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

std::vector<char> game::file_loader::readFileBinary (const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        spdlog::get ("main_out")->critical ("Failed to open file '{}'", filename);
        throw std::runtime_error ("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg (0);
    file.read (buffer.data(), fileSize);

    file.close();
    return buffer;
}

game::file_loader::TextureData game::file_loader::readFileImage (const std::string &filename) {
    TextureData retData = {};
    retData.data = stbi_load (filename.c_str(), &retData.width, &retData.height, &retData.channels, STBI_rgb_alpha);
    if (!retData.data) {
        auto err = stbi_failure_reason();
        spdlog::critical ("Failed to load image from '{}' because: {}", filename, err);
        throw std::runtime_error(err);
    }

    return retData;
}

void game::file_loader::freeImageData (game::file_loader::TextureData data) {
    stbi_image_free (data.data);
}
