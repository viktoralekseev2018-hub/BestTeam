#include "config.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Config Config::load(const std::string& path) {
    Config cfg;
    cfg.config_path = path;
    cfg.setDefaults();

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл конфигурации: " + path);
    }

    try {
        json j;
        file >> j;

        if (j.contains("uid")) cfg.uid = j["uid"].get<std::string>();
        if (j.contains("server_url")) cfg.server_url = j["server_url"].get<std::string>();
        if (j.contains("access_code")) cfg.access_code = j["access_code"].get<std::string>();
        if (j.contains("tasks_folder")) cfg.tasks_folder = j["tasks_folder"].get<std::string>();
        if (j.contains("results_folder")) cfg.results_folder = j["results_folder"].get<std::string>();
        if (j.contains("log_file")) cfg.log_file = j["log_file"].get<std::string>();
        if (j.contains("poll_interval")) cfg.poll_interval = std::chrono::seconds(j["poll_interval"]);
        if (j.contains("max_poll_interval")) cfg.max_poll_interval = std::chrono::seconds(j["max_poll_interval"]);
        if (j.contains("timeout")) cfg.timeout = std::chrono::seconds(j["timeout"]);
        if (j.contains("max_retries")) cfg.max_retries = j["max_retries"];
        if (j.contains("retry_delay")) cfg.retry_delay = std::chrono::seconds(j["retry_delay"]);

    } catch (const json::exception& e) {
        throw std::runtime_error("Ошибка парсинга JSON: " + std::string(e.what()));
    }

    if (!cfg.validate()) {
        throw std::runtime_error("Проверка конфигурации не пройдена");
    }

    return cfg;
}

void Config::save(const std::string& path) const {
    json j;
    j["uid"] = uid;
    j["server_url"] = server_url;
    j["access_code"] = access_code;
    j["tasks_folder"] = tasks_folder.string();
    j["results_folder"] = results_folder.string();
    j["log_file"] = log_file.string();
    j["poll_interval"] = poll_interval.count();
    j["max_poll_interval"] = max_poll_interval.count();
    j["timeout"] = timeout.count();
    j["max_retries"] = max_retries;
    j["retry_delay"] = retry_delay.count();

    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл для записи: " + path);
    }
    file << j.dump(4);
}

void Config::setDefaults() {
    uid = "agent-" + std::to_string(std::time(nullptr));
    access_code.clear();
    tasks_folder = std::filesystem::current_path() / "tasks";
    results_folder = std::filesystem::current_path() / "results";
    log_file = std::filesystem::current_path() / "agent.log";
}

bool Config::validate() const {
    if (uid.empty() || server_url.empty()) return false;

    try {
        std::filesystem::create_directories(tasks_folder);
        std::filesystem::create_directories(results_folder);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }

    return true;
}