//
// Created by Peter Lewis on 2022-07-06.
//

#ifndef VULKANENGINE_ENGINEUTILS_H
#define VULKANENGINE_ENGINEUTILS_H

#include <functional>

namespace engine {

    // from: https://stackoverflow.com/a/57595105
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    }

} // engine

#endif //VULKANENGINE_ENGINEUTILS_H
