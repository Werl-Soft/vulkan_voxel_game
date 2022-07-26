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

    spdlog::logger logger("main_out", {consoleSink, fileSink});
    spdlog::register_logger (std::make_shared<spdlog::logger>(logger));
    logger.set_level (spdlog::level::debug);
    logger.enable_backtrace (32);
    spdlog::set_default_logger (std::make_shared<spdlog::logger>(logger));

    engine::FirstApp app{};

    try {
        logger.info("Starting Application");
        app.run ();
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
