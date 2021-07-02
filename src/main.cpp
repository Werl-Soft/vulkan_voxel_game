#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

class HelloTriangleApplication {
public:
    void run() {
        preInit ();
        initVulkan ();
        mainLoop ();
        cleanup ();
    }

private:
    void preInit() {

    }

    void initVulkan() {

    }

    void mainLoop() {

    }

    void cleanup() {

    }
};

int main() {
    HelloTriangleApplication app;

    auto stdLogger = spdlog::stdout_color_mt ("console");
    auto errLogger = spdlog::stdout_color_mt ("stderr");

    try {
        stdLogger->info ("Starting Application");
        app.run ();
        stdLogger->info ("Stopping Application");
    } catch (std::exception& e) {
        stdLogger->critical ("Application Crashed: {}", e.what ());
        errLogger->critical ("Application Crashed: {}", e.what ());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
