//
// Created by peter on 2021-12-20.
//

#ifndef BASIC_TESTS_FILE_LOADER_HPP
#define BASIC_TESTS_FILE_LOADER_HPP

#include <fstream>
#include <vector>

namespace game::file_loader{
    std::vector<char> readFile (const std::string &filename);
}
#endif //BASIC_TESTS_FILE_LOADER_HPP