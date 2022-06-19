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
        glfwInit();
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
            throw std::runtime_error("failed to create window surface");
        }
    }

    void EngineWindow::framebufferResizedCallback (GLFWwindow *window, int width, int height) {
        auto engineWindow = reinterpret_cast<EngineWindow *>(glfwGetWindowUserPointer (window));
        engineWindow->framebufferResized = true;
        engineWindow->width = width;
        engineWindow->height = height;
    }
}
