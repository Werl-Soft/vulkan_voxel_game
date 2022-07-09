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
#include <unordered_map>

namespace engine {

    struct TransformComponent {
        glm::vec3 translation {}; // position offset
        glm::vec3 scale {1.0f, 1.0f, 1.0f};
        glm::vec3 rotation;

        // Matrix corresponds to translate * Ry * Rx * Rz * scale transformation
        // Rotation convention uses tait-bryan angles with the axis order Y(1), X(2), Z(3)
        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };

    class EngineGameObject {
    public :
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, EngineGameObject>;

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
