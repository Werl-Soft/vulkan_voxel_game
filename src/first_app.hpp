//
// Created by Peter Lewis on 2022-05-18.
//

#ifndef BASIC_TESTS_FIRST_APP_HPP
#define BASIC_TESTS_FIRST_APP_HPP

#include "engine_window.hpp"
#include "engine_device.hpp"
#include "engine_game_object.hpp"
#include "engine_renderer.hpp"
#include "engine_buffer.hpp"
#include "engine_descriptors.hpp"

// std
#include <memory>

namespace engine {

    class FirstApp {
    public:
        FirstApp ();
        virtual ~FirstApp ();

        FirstApp(const FirstApp &) = delete;
        FirstApp operator=(const FirstApp &) = delete;

        void run();

    private:
        void loadGameObjects();

        EngineWindow engineWindow{800, 600, "Hello Vulkan!"};
        EngineDevice engineDevice{engineWindow};
        EngineRenderer engineRenderer{engineWindow, engineDevice};

        // note: order of declarations matter
        std::unique_ptr<EngineDescriptorPool> globalPool{};
        EngineGameObject::Map gameObjects;
    };
}

#endif //BASIC_TESTS_FIRST_APP_HPP
