//
// Created by Peter Lewis on 2022-05-18.
//

#include "first_app.hpp"
#include "simple_render_system.hpp"
#include "engine_camera.hpp"
#include "keyboard_movement_controller.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <chrono>

namespace engine {
    void FirstApp::run () {
        SimpleRenderSystem simpleRenderSystem{engineDevice, engineRenderer.getSwapchainRenderpass()};
        EngineCamera camera {};
        camera.setViewTarget (glm::vec3(-1.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 2.5f));

        auto viewerObject = EngineGameObject::createGameObject();
        KeyboardMovementController cameraController {};

        auto currentTime = std::chrono::high_resolution_clock::now();

        while (!engineWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime =  std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ (engineWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ (viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = engineRenderer.getAspectRatio();

            camera.setPerspectiveProjection (glm::radians (50.0f), aspect, 0.1f, 10.0f);

            if (auto commandBuffer = engineRenderer.beginFrame()) {
                engineRenderer.beginSwapChainRenderPass (commandBuffer);
                simpleRenderSystem.renderGameObjects (commandBuffer, gameObjects, camera);
                engineRenderer.endSwapChainRenderPass (commandBuffer);
                engineRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle (engineDevice.device());
    }

    FirstApp::FirstApp () {
        loadGameObjects ();
    }

    FirstApp::~FirstApp () {
    }

    // temporary helper function, creates a 1x1x1 cube centered at offset with an index buffer
    std::unique_ptr<EngineModel> createCubeModel(EngineDevice& device, glm::vec3 offset) {
        EngineModel::Builder modelBuilder{};
        modelBuilder.vertices = {
                // left face (white)
                {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
                {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
                {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
                {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},

                // right face (yellow)
                {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
                {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
                {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
                {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},

                // top face (orange, remember y axis points down)
                {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
                {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
                {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
                {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},

                // bottom face (red)
                {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
                {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
                {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
                {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},

                // nose face (blue)
                {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
                {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
                {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
                {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},

                // tail face (green)
                {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
                {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
                {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
                {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
        };
        for (auto& v : modelBuilder.vertices) {
            v.position += offset;
        }

        modelBuilder.indices = {0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
                                12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21};

        return std::make_unique<EngineModel>(device, modelBuilder);
    }

    void FirstApp::loadGameObjects () {
        std::shared_ptr<EngineModel> engineModel = createCubeModel (engineDevice, {0.0f, 0.0f, 0.0f});

        auto cube = EngineGameObject::createGameObject();
        cube.model = engineModel;
        cube.transform.translation = {0.0f, 0.0f, 2.5f};
        cube.transform.scale = {0.5f, 0.5f, 0.5f};

        gameObjects.push_back (std::move (cube));
    }
} // engine
