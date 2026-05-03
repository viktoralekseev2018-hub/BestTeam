#include "config.h"
#include "agent.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <csignal>
#include <iostream>
#include <memory>

std::unique_ptr<WebAgent> g_agent;

void signalHandler(int signum) {
    spdlog::info("Получен сигнал {}, остановка агента...", signum);
    if (g_agent) {
        g_agent->stop();
    }
    spdlog::info("Агент остановлен");
    exit(signum);
}

int main(int argc, char* argv[]) {
    std::string config_path = "config.json";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-c" && i + 1 < argc) {
            config_path = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "WebAgent - Агент для выполнения распределённых заданий\n"
                      << "Использование: " << argv[0] << " [-c файл_конфига]\n"
                      << "Параметры:\n"
                      << "  -c <файл>  Путь к файлу конфигурации\n"
                      << "  -h         Показать эту справку\n";
            return 0;
        }
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("agent.log", true);
    file_sink->set_level(spdlog::level::debug);

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("web-agent", sinks.begin(), sinks.end());
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("WebAgent запускается...");

    try {
        Config config = Config::load(config_path);
        spdlog::info("Конфигурация загружена: UID={}, Сервер={}", config.uid, config.server_url);

        g_agent = std::make_unique<WebAgent>(config);

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        g_agent->start();

        while (g_agent->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const std::exception& e) {
        spdlog::critical("Критическая ошибка: {}", e.what());
        return 1;
    }

    spdlog::info("WebAgent завершен");
    return 0;
}