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

    void FirstApp::loadGameObjects () {
        std::shared_ptr<EngineModel> engineModel = EngineModel::createModelFromFile (engineDevice, "assets/models/smooth_vase.obj");

        auto gameObj = EngineGameObject::createGameObject();
        gameObj.model = engineModel;
        gameObj.transform.translation = {0.0f, 0.5f, 2.5f};
        gameObj.transform.scale = glm::vec3 (3.0f);

        gameObjects.push_back (std::move (gameObj));
    }
} // engine
