//
// Created by Peter Lewis on 2022-05-18.
//

#include "engine_window.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace engine {

    EngineWindow::EngineWindow (int w, int h, std::string name) : width{w}, height{h}, windowName{std::move(name)}{
        initWindow();
    }

    EngineWindow::~EngineWindow () {
        glfwDestroyWindow (window);
        glfwTerminate();
    }

    void EngineWindow::initWindow () {

        int major, minor, rev;
        glfwGetVersion (&major, &minor, &rev);
        if (major < 3 || minor < 2) {
            spdlog::critical("GLFW version {}.{}.{} is not supported, please install at least 3.2", major, minor, rev);
            throw std::runtime_error ("GLFW version too old, please use at least 3.2");
        }
        if (major > 3) {
            spdlog::warn ("GLFW version {}.{}.{} is unsupported, things may not work as intended", major, minor, rev);
        }

        glfwSetErrorCallback (glfwErrorCallback);
        if (glfwInit() != GLFW_TRUE) {
            spdlog::critical("GLFW failed to initialize");
            throw std::runtime_error("GLFW failed to initialize");
        }
        glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint (GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer (window, this);
        glfwSetWindowSizeCallback (window, framebufferResizedCallback);
    }

    bool EngineWindow::shouldClose () {
        return glfwWindowShouldClose (window);
    }

    void EngineWindow::createWindowSurface (VkInstance instance, VkSurfaceKHR *surface) {
        if (glfwCreateWindowSurface (instance, window, nullptr, surface) != VK_SUCCESS) {
            spdlog::critical ("Failed to create window surface");
            throw std::runtime_error("Failed to create window surface");
        }
    }

    void EngineWindow::framebufferResizedCallback (GLFWwindow *window, int width, int height) {
        auto engineWindow = reinterpret_cast<EngineWindow *>(glfwGetWindowUserPointer (window));
        engineWindow->framebufferResized = true;
        engineWindow->width = width;
        engineWindow->height = height;
    }

    void EngineWindow::glfwErrorCallback (int error_code, const char *description) {
        spdlog::error ("GLFW Error [{}]: {}", error_code, description);
    }
}
