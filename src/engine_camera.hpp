//
// Created by Peter Lewis on 2022-07-01.
//

#ifndef VULKANENGINE_ENGINE_CAMERA_HPP
#define VULKANENGINE_ENGINE_CAMERA_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {
    class EngineCamera {
    public:
        void setOrthographicProjection (float left, float right, float top, float bottom, float near, float far);
        void setPerspectiveProjection (float fovY, float aspect, float near, float far);

        void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
        void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, -1.0f, 0.0f});
        void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

        [[nodiscard]] const glm::mat4& getProjection() const { return projectionMatrix; }
        [[nodiscard]] const glm::mat4 &getViewMatrix () const;

    private:
        glm::mat4 projectionMatrix{1.0f};
        glm::mat4 viewMatrix{1.0f};
    };
} // engine


#endif //VULKANENGINE_ENGINE_CAMERA_HPP
