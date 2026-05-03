#pragma once

#include <string>
#include <filesystem>
#include <chrono>

struct Config {
    std::string uid;
    std::string server_url;
    std::string access_code;
    std::filesystem::path tasks_folder;
    std::filesystem::path results_folder;
    std::filesystem::path log_file;
    std::chrono::seconds poll_interval{5};
    std::chrono::seconds max_poll_interval{300};
    std::chrono::seconds timeout{30};
    int max_retries{3};
    std::chrono::seconds retry_delay{5};
    std::string config_path;

    static Config load(const std::string& path);
    void save(const std::string& path) const;
    bool validate() const;
    void setDefaults();
};