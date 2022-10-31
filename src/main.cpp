#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include "first_app.hpp"

#include <cstdlib>
#include <stdexcept>

int main(int argc, char* argv[]) {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level (spdlog::level::info);

    auto max_size = 1048576 * 50;
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/log.txt", max_size, 5, true);
    fileSink->set_level (spdlog::level::trace);

    spdlog::logger logger("main", {consoleSink, fileSink});
    logger.enable_backtrace (32);
    logger.set_level (spdlog::level::trace);
    spdlog::register_logger (std::make_shared<spdlog::logger>(logger));
    spdlog::set_default_logger (spdlog::get ("main"));

    spdlog::register_logger (spdlog::get ("main")->clone ("vulkan"));
    spdlog::register_logger (spdlog::get ("main")->clone ("renderer"));
    spdlog::register_logger (spdlog::get ("main")->clone ("assets"));

    try {
        logger.info("Starting Application");
        engine::FirstApp app{};

        app.run();
        logger.info ("Stopping Application");
    } catch (std::exception& e) {
        logger.critical ("Application Crashed: {}", e.what ());
        logger.dump_backtrace();
        logger.flush();
        spdlog::shutdown();
        return EXIT_FAILURE;
    } catch (...) {
        logger.critical ("Application Crashed To Unknown Error");
        logger.dump_backtrace();
        logger.flush();
        spdlog::shutdown();
        return EXIT_FAILURE;
    }
    logger.flush();
    return EXIT_SUCCESS;
}
