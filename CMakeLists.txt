cmake_minimum_required(VERSION 3.15)
project(Basic_Tests)

set(CMAKE_CXX_STANDARD 20)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(libs/glfw)

# Find spdlog
add_subdirectory(libs/spdlog)

# Find Boost
#find_package(Boost 1.76 REQUIRED COMPONENTS system container)

# Find GLM
add_subdirectory(libs/glm)

# Find tinygltf
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_NO_INCLUDE_STB_IMAGE ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_NO_EXTERNAL_IMAGE OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_USE_CPP14 ON CACHE INTERNAL "" FORCE)
add_subdirectory(libs/tinygltf)

# Find Others
include_directories(libs/other/include)

add_executable(Basic_Tests src/main.cpp src/vulkan_helpers.cpp src/vulkan_helpers.hpp src/file_loader.cpp src/file_loader.hpp src/game_application.cpp src/game_application.hpp)

# Link Libraries
target_link_libraries(Basic_Tests PUBLIC Vulkan::Vulkan glm::glm tinygltf)
target_link_libraries(Basic_Tests PUBLIC glfw)
target_link_libraries(Basic_Tests PRIVATE spdlog::spdlog)


add_custom_command(TARGET Basic_Tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory assets
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/textures
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/models)

add_custom_command(TARGET Basic_Tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/textures assets/textures)

add_custom_command(TARGET Basic_Tests POST_BUILD
        COMMAND glslc ${CMAKE_SOURCE_DIR}/assets/shaders/fragment.frag -o assets/shaders/frag.spv
        COMMAND glslc ${CMAKE_SOURCE_DIR}/assets/shaders/vertex.vert -o assets/shaders/vert.spv)
