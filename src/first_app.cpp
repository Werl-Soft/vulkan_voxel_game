//
// Created by Peter Lewis on 2022-05-18.
//

#include "first_app.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "engine_camera.hpp"
#include "keyboard_movement_controller.hpp"
#include "engine_texture.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <chrono>

namespace engine {
    void FirstApp::run () {
        std::vector<std::unique_ptr<EngineBuffer>> uboBuffers(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);

        for (auto & uboBuffer : uboBuffers) {
            uboBuffer = std::make_unique<EngineBuffer>(
                    engineDevice,
                    sizeof (GlobalUBO),
                    1,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffer->map ();
        }

        EngineBuffer globalUBOBuffer {
            engineDevice,
            sizeof(GlobalUBO),
            EngineSwapChain::MAX_FRAMES_IN_FLIGHT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            engineDevice.properties.limits.minUniformBufferOffsetAlignment
        };
        globalUBOBuffer.map ();

        auto globalSetLayout = EngineDescriptorSetLayout::Builder(engineDevice)
                .addBinding (0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                .addBinding (1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();

        EngineTexture texture = EngineTexture(engineDevice, "assets/textures/statue.jpg");

        VkDescriptorImageInfo imageInfo {};
        imageInfo.sampler = texture.getSampler();
        imageInfo.imageView = texture.getImageView();
        imageInfo.imageLayout = texture.getImageLayout();

        std::vector<VkDescriptorSet> globalDescriptorSets (EngineSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo ();
            EngineDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer (0, &bufferInfo)
                .writeImage (1, &imageInfo)
                .build (globalDescriptorSets[i]);
        }

        system::SimpleRenderSystem simpleRenderSystem{engineDevice, engineRenderer.getSwapchainRenderpass(), globalSetLayout->getDescriptorSetLayout()};
        system::PointLightSystem pointLightSystem{engineDevice, engineRenderer.getSwapchainRenderpass(), globalSetLayout->getDescriptorSetLayout()};

        EngineCamera camera {};
        camera.setViewTarget (glm::vec3(-1.0f, -2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 2.5f));

        auto viewerObject = EngineGameObject::createGameObject();
        viewerObject.transform.translation.z = -2.5f;
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

            camera.setPerspectiveProjection (glm::radians (50.0f), aspect, 0.1f, 100.0f);

            if (auto commandBuffer = engineRenderer.beginFrame()) {
                int frameIndex = engineRenderer.getFrameIndex();
                EngineFrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    gameObjects
                };

                //update
                GlobalUBO ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getViewMatrix();
                ubo.inverseView = camera.getInverseViewMatrix();
                pointLightSystem.update (frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer (&ubo);
                uboBuffers[frameIndex]->flush();

                //render
                engineRenderer.beginSwapChainRenderPass (commandBuffer);
                simpleRenderSystem.renderGameObjects (frameInfo);
                pointLightSystem.render (frameInfo);
                engineRenderer.endSwapChainRenderPass (commandBuffer);
                engineRenderer.endFrame();
            }
        }
        vkDeviceWaitIdle (engineDevice.device());
    }

    FirstApp::FirstApp () {

        globalPool = EngineDescriptorPool::Builder(engineDevice)
                .setMaxSets (EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EngineSwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();
        loadGameObjects ();
    }

    FirstApp::~FirstApp () = default;

    void FirstApp::loadGameObjects () {
        std::shared_ptr<EngineModel> engineModel = EngineModel::createModelFromFile (engineDevice, "assets/models/smooth_vase.obj");
        auto gameObj = EngineGameObject::createGameObject();
        gameObj.model = engineModel;
        gameObj.transform.translation = {-0.5f, 0.5f, 0.0f};
        gameObj.transform.scale = glm::vec3 (3.0f);
        gameObjects.emplace(gameObj.getId(), std::move (gameObj));

        engineModel = EngineModel::createModelFromFile (engineDevice, "assets/models/flat_vase.obj");
        auto flatVase = EngineGameObject::createGameObject();
        flatVase.model = engineModel;
        flatVase.transform.translation = {0.5f, 0.5f, 0.0f};
        flatVase.transform.scale = {3.0f, 1.5f, 3.0f};
        gameObjects.emplace(flatVase.getId(), std::move(flatVase));



        //engineModel = EngineModel::createModelFromFile (engineDevice, "assets/models/quad.obj");
        engineModel = EngineModel::createModelFromNoise (engineDevice, 16, 16);
        auto floor = EngineGameObject::createGameObject();
        floor.model = engineModel;
        floor.transform.translation = {-8.0f, 0.5f, -2.0f};
        floor.transform.scale = {1.0f, 1.0f, 1.0f};
        gameObjects.emplace(floor.getId(), std::move(floor));

        std::vector<glm::vec3> lightColors{
                {1.f, .1f, .1f},
                {.1f, .1f, 1.f},
                {.1f, 1.f, .1f},
                {1.f, 1.f, .1f},
                {.1f, 1.f, 1.f},
                {1.f, 1.f, 1.f}  //
        };

        for (int i = 0; i < lightColors.size(); i++) {
            auto pointLight = EngineGameObject::makePointLight (0.2);
            pointLight.color = lightColors[i];
            auto rotateLight = glm::rotate (glm::mat4(1.0f), i * glm::two_pi<float>() / lightColors.size(), {0.0f, -1.0f, 0.0f});
            pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f));

            gameObjects.emplace (pointLight.getId(), std::move (pointLight));
        }
    }
} // engine
