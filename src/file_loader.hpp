//
// Created by peter on 2021-12-20.
//

#ifndef BASIC_TESTS_FILE_LOADER_HPP
#define BASIC_TESTS_FILE_LOADER_HPP

#include <vector>
#include <string>

namespace game::file_loader{

    std::vector<char> readFileBinary (const std::string &filename);

}
#endif //BASIC_TESTS_FILE_LOADER_HPP
