//
// Created by Peter Lewis on 2022-05-18.
//

#include "first_app.hpp"
#include "simple_render_system.hpp"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace engine {
    void FirstApp::run () {
        SimpleRenderSystem simpleRenderSystem{engineDevice, engineRenderer.getSwapchainRenderpass()};

        while (!engineWindow.shouldClose()) {
            glfwPollEvents();
            if (auto commandBuffer = engineRenderer.beginFrame()) {
                engineRenderer.beginSwapChainRenderPass (commandBuffer);
                simpleRenderSystem.renderGameObjects (commandBuffer, gameObjects);
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
        std::vector<EngineModel::Vertex> vertices {
                {{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
                {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
        };

        auto engineModel = std::make_shared<EngineModel>(engineDevice, vertices);

        auto triangle = EngineGameObject::createGameObject();
        triangle.model = engineModel;
        triangle.color = {0.1f, 0.8f, 0.1f};
        triangle.tarnsform2D.translation.x = 0.2f;
        triangle.tarnsform2D.scale = {2.0f, 0.5f};
        triangle.tarnsform2D.rotation = 0.25f * glm::two_pi<float>();

        gameObjects.push_back (std::move(triangle));
    }
} // engine
