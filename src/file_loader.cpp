//
// Created by peter on 2021-12-20.
//

#include "file_loader.hpp"
#include <spdlog/spdlog.h>

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