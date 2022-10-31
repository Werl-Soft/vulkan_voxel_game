//
// Created by Peter Lewis on 2022-05-18.
//

#ifndef BASIC_TESTS_ENGINE_WINDOW_HPP
#define BASIC_TESTS_ENGINE_WINDOW_HPP

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace engine {

    class EngineWindow {
    public:
        EngineWindow(int width, int height, std::string name);
        ~EngineWindow();

        EngineWindow(const EngineWindow &) = delete;
        EngineWindow &operator=(const EngineWindow &) = delete;

        bool shouldClose ();
        [[nodiscard]] VkExtent2D getExtent() const { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
        [[nodiscard]] bool wasWindowResized() const { return framebufferResized; }
        void resetWindowResizedFlag() { framebufferResized = false; }
        [[nodiscard]] GLFWwindow *getGLFWwindow() const { return window; }

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    private:
        int width;
        int height;
        bool framebufferResized = false;

        std::string windowName;
        GLFWwindow *window;

        static void framebufferResizedCallback (GLFWwindow *window, int width, int height);
        static void glfwErrorCallback(int error_code, const char* description);
        void initWindow();

    };

}

#endif //BASIC_TESTS_ENGINE_WINDOW_HPP
