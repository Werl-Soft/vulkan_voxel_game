//
// Created by Peter Lewis on 2022-06-10.
//

#ifndef BASIC_TESTS_ENGINE_GAME_OBJECT_HPP
#define BASIC_TESTS_ENGINE_GAME_OBJECT_HPP

#include "engine_model.hpp"

// libs
#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>

namespace engine {

    struct TransformComponent {
        glm::vec3 translation {}; // position offset
        glm::vec3 scale {1.0f, 1.0f, 1.0f};
        glm::vec3 rotation;

        // Matrix corresponds to translate * Ry * Rx * Rz * scale transformation
        // Rotation convention uses tait-bryan angles with the axis order Y(1), X(2), Z(3)
        glm::mat4 mat4() {
            const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);
            return glm::mat4{
                    {
                            scale.x * (c1 * c3 + s1 * s2 * s3),
                            scale.x * (c2 * s3),
                            scale.x * (c1 * s2 * s3 - c3 * s1),
                            0.0f,
                    },
                    {
                            scale.y * (c3 * s1 * s2 - c1 * s3),
                            scale.y * (c2 * c3),
                            scale.y * (c1 * c3 * s2 + s1 * s3),
                            0.0f,
                    },
                    {
                            scale.z * (c2 * s1),
                            scale.z * (-s2),
                            scale.z * (c1 * c2),
                            0.0f,
                    },
                    {translation.x, translation.y, translation.z, 1.0f}};
        }
    };

    class EngineGameObject {
    public :
        using id_t = unsigned int;

    static EngineGameObject createGameObject() {
        static id_t currentId = 0;
        return EngineGameObject{currentId++};
    };

    EngineGameObject (const EngineGameObject &) = delete;
    EngineGameObject &operator=(const EngineGameObject &) = delete;
    EngineGameObject (EngineGameObject &&) = default;
    EngineGameObject &operator=(EngineGameObject &&) = delete;

    id_t getId() { return id; };

    std::shared_ptr<EngineModel> model {};
    glm::vec3 color {};
    TransformComponent transform {};

    private:
        EngineGameObject(id_t objID) : id{objID} {}

        id_t id;
    };

} // engine

#endif //BASIC_TESTS_ENGINE_GAME_OBJECT_HPP
