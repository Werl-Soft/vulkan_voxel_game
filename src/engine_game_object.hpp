//
// Created by Peter Lewis on 2022-06-10.
//

#ifndef BASIC_TESTS_ENGINE_GAME_OBJECT_HPP
#define BASIC_TESTS_ENGINE_GAME_OBJECT_HPP

#include "engine_model.hpp"

// std
#include <memory>

namespace engine {

    struct Transform2DComponent {
        glm::vec2 translation {}; // position offset
        glm::vec2 scale {1.0f, 1.0f};
        float rotation;
        glm::mat2 mat2() {
            const float s = glm::sin (rotation);
            const float c = glm::cos (rotation);
            glm::mat2 rotMatrix {{c, s}, {-s, c}};

            glm::mat2 scaleMat {{scale.x, 0.0f}, {0.0f, scale.y}};
            return rotMatrix * scaleMat;
        };
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
    Transform2DComponent tarnsform2D {};

    private:
        EngineGameObject(id_t objID) : id{objID} {}

        id_t id;
    };

} // engine

#endif //BASIC_TESTS_ENGINE_GAME_OBJECT_HPP
