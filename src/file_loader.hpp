//
// Created by peter on 2021-12-20.
//

#ifndef BASIC_TESTS_FILE_LOADER_HPP
#define BASIC_TESTS_FILE_LOADER_HPP

#include <vector>
#include <string>

namespace game::file_loader{

    struct TextureData {
        unsigned char *data;
        int width;
        int height;
        int channels;

        [[nodiscard]] int getSize() const {
            return width * height * 4;
        }
    };

    std::vector<char> readFileBinary (const std::string &filename);

    TextureData readFileImage(const std::string &filename);

    void freeImageData(TextureData data);

}
#endif //BASIC_TESTS_FILE_LOADER_HPP
