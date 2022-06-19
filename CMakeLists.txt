cmake_minimum_required(VERSION 3.15)
project(VulkanEngine)

set(CMAKE_CXX_STANDARD 20)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

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

# Create Executable
add_executable(Engine_App src/main.cpp src/engine_window.cpp src/engine_window.hpp src/first_app.cpp src/first_app.hpp src/engine_pipeline.cpp src/engine_pipeline.hpp src/engine_device.cpp src/engine_device.hpp src/engine_swapchain.cpp src/engine_swapchain.hpp src/engine_model.cpp src/engine_model.hpp src/engine_game_object.cpp src/engine_game_object.hpp src/engine_renderer.cpp src/engine_renderer.hpp src/simple_render_system.cpp src/simple_render_system.hpp)
add_dependencies(Engine_App BuildShaders)
# Link Libraries

target_link_libraries(Engine_App PUBLIC Vulkan::Vulkan glm::glm)
target_link_libraries(Engine_App PUBLIC glfw)
target_link_libraries(Engine_App PRIVATE spdlog::spdlog)

add_custom_command(TARGET Engine_App POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory assets
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/textures
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/models)

# Move Assets
add_custom_command(TARGET Engine_App POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/textures assets/textures
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/models assets/models)

# Create Shaders
add_custom_target(BuildShaders
        COMMAND glslc ${CMAKE_SOURCE_DIR}/assets/shaders/simple_shader.frag -o assets/shaders/simple_shader.frag.spv
        COMMAND glslc ${CMAKE_SOURCE_DIR}/assets/shaders/simple_shader.vert -o assets/shaders/simple_shader.vert.spv
        BYPRODUCTS assets/shaders/simple_shader.frag assets/shaders/simple_shader.vert)

add_custom_command(TARGET Engine_App POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:Engine_App> $<TARGET_FILE_DIR:Engine_App>
        COMMAND_EXPAND_LISTS
        )
