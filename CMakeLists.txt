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

# Find json.hpp
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(libs/json)

# Find FastNoise2
set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE)
add_subdirectory(libs/FastNoise2)

# Find Others
include_directories(libs/other/include)

# Create Executable
add_executable(Engine_App src/main.cpp src/engine_window.cpp src/engine_window.hpp src/first_app.cpp src/first_app.hpp src/engine_pipeline.cpp src/engine_pipeline.hpp src/engine_device.cpp src/engine_device.hpp src/engine_swapchain.cpp src/engine_swapchain.hpp src/engine_model.cpp src/engine_model.hpp src/engine_game_object.cpp src/engine_game_object.hpp src/engine_renderer.cpp src/engine_renderer.hpp src/systems/simple_render_system.cpp src/systems/simple_render_system.hpp src/engine_camera.cpp src/engine_camera.hpp src/keyboard_movement_controller.cpp src/keyboard_movement_controller.hpp src/engine_utils.cpp src/engine_utils.hpp src/engine_buffer.cpp src/engine_buffer.hpp src/engine_frame_info.cpp src/engine_frame_info.hpp src/engine_descriptors.cpp src/engine_descriptors.hpp src/systems/point_light_system.cpp src/systems/point_light_system.hpp)
add_dependencies(Engine_App BuildShaders)

# Link Libraries
target_link_libraries(Engine_App PUBLIC Vulkan::Vulkan glm::glm nlohmann_json::nlohmann_json FastNoise)
target_link_libraries(Engine_App PUBLIC glfw)
target_link_libraries(Engine_App PRIVATE spdlog::spdlog)

add_custom_target(MakeDirectoryStructure
        COMMAND ${CMAKE_COMMAND} -E make_directory assets
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/textures
        COMMAND ${CMAKE_COMMAND} -E make_directory assets/models)

# Move Assets
add_custom_target(CopyAssets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/textures assets/textures
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/models assets/models)
add_dependencies(CopyAssets MakeDirectoryStructure)

# Create Shaders
file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag")

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "assets/shaders/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND glslc ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
            BYPRODUCTS ${SPIRV})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(BuildShaders DEPENDS ${SPIRV_BINARY_FILES})

# Copy dynamic libs
add_custom_command(TARGET Engine_App POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:Engine_App> $<TARGET_FILE_DIR:Engine_App>
        COMMAND_EXPAND_LISTS)
