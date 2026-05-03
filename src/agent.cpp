#include "agent.h"
#include "logger.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

using json = nlohmann::json;

// ============================================
// Task
// ============================================

bool Task::isValid() const {
    if (type == TaskType::None || session_id.empty() || task_id.empty()) return false;

    switch (type) {
        case TaskType::ExecuteCommand:
            return !command.empty();
        case TaskType::RunProgram:
            return !program_path.empty();
        case TaskType::TransferFile:
            return !source_files.empty() && !destination_folder.empty();
        default:
            return true;
    }
}

Task Task::fromJson(const std::string& json_str) {
    Task task;
    auto j = json::parse(json_str);

    task.session_id = j.value("session_id", "");
    task.task_id = j.value("task_id", "");

    std::string type_str = j.value("type", "none");
    if (type_str == "execute_command") {
        task.type = TaskType::ExecuteCommand;
        task.command = j.value("command", "");
        task.timeout = std::chrono::seconds(j.value("timeout", 300));
    } else if (type_str == "run_program") {
        task.type = TaskType::RunProgram;
        task.program_path = j.value("program", "");
        if (j.contains("arguments")) {
            task.arguments = j["arguments"].get<std::vector<std::string>>();
        }
        task.timeout = std::chrono::seconds(j.value("timeout", 300));
    } else if (type_str == "transfer_file") {
        task.type = TaskType::TransferFile;
        task.source_files = j.value("source_files", std::vector<std::string>{});
        task.destination_folder = j.value("destination", "");
    }

    return task;
}

// ============================================
// WebAgent
// ============================================

WebAgent::WebAgent(const Config& config)
    : config_(config)
    , http_client_(config.timeout)
    , current_interval_(config.poll_interval) {

    logger_ = std::make_unique<Logger>(config.log_file);
    http_client_.setVerifySsl(config.server_url.find("https") == 0);
    executor_config_ = std::make_shared<Config>(config);

    spdlog::info("WebAgent инициализирован с UID: {}", config_.uid);
}

WebAgent::~WebAgent() {
    stop();
}

void WebAgent::start() {
    if (running_) return;
    running_ = true;
    worker_thread_ = std::thread(&WebAgent::workerLoop, this);
    spdlog::info("WebAgent запущен");
}

void WebAgent::stop() {
    if (!running_) return;
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    spdlog::info("WebAgent остановлен");
}

void WebAgent::workerLoop() {
    if (config_.access_code.empty()) {
        if (!registerAgent()) {
            spdlog::error("Не удалось зарегистрировать агент");
            handleNetworkError();
        }
    } else {
        spdlog::info("Используем существующий access_code");
        session_id_ = config_.access_code;
    }

    while (running_) {
        try {
            auto task_opt = fetchTask();

            if (task_opt.has_value()) {
                Task task = task_opt.value();
                spdlog::info("Получено задание: {}", task.task_code);

                ExecutionResult result = executeTask(task);

                if (!sendResult(task, result)) {
                    spdlog::error("Не удалось отправить результаты для задания {}", task.task_id);
                }

                if (result.success) {
                    spdlog::info("Задание {} выполнено успешно", task.task_code);
                    logger_->logTask(task, result);
                } else {
                    spdlog::error("Задание {} не выполнено: {}", task.task_code, result.error_message);
                    logger_->logError(task.task_id, result.error_message);
                }

                current_interval_ = config_.poll_interval;
                consecutive_failures_ = 0;
            } else {
                spdlog::info("Нет заданий, следующий опрос через {} секунд", current_interval_.count());
                std::this_thread::sleep_for(current_interval_);
            }

        } catch (const std::exception& e) {
            spdlog::error("Исключение в цикле работы: {}", e.what());
            handleNetworkError();
        }
    }
}

bool WebAgent::registerAgent() {
    spdlog::info("Регистрация агента с UID: {}", config_.uid);

    json request;
    request["UID"] = config_.uid;
    request["descr"] = "web-agent";

    std::string url = config_.server_url;
    if (url.back() != '/') url += '/';
    url += "wa_reg/";

    auto response = http_client_.post(url, request.dump());

    if (!response.success) {
        spdlog::error("Ошибка регистрации: {}", response.error);
        return false;
    }

    try {
        auto response_json = json::parse(response.body);
        int code = response_json.value("code_responce", -1);

        if (code == 0) {
            session_id_ = response_json.value("access_code", "");
            if (!session_id_.empty()) {
                spdlog::info("Регистрация успешна, access_code: {}", session_id_);
                return true;
            }
        }

        std::string msg = response_json.value("msg", "unknown error");
        spdlog::error("Ошибка регистрации: {} (code {})", msg, code);
        return false;
    } catch (const json::exception& e) {
        spdlog::error("Не удалось разобрать ответ регистрации: {}", e.what());
        return false;
    }
}

std::optional<Task> WebAgent::fetchTask() {
    if (session_id_.empty()) {
        spdlog::error("Нет access_code, невозможно получить задание");
        return std::nullopt;
    }

    json request;
    request["UID"] = config_.uid;
    request["descr"] = "web-agent";
    request["access_code"] = session_id_;

    std::string url = config_.server_url;
    if (url.back() != '/') url += '/';
    url += "wa_task/";

    auto response = http_client_.post(url, request.dump());

    if (!response.success) {
        throw std::runtime_error("Не удалось получить задание: " + response.error);
    }

    try {
        auto resp_json = json::parse(response.body);

        int code = -1;
        if (resp_json.contains("code_responce")) {
            auto val = resp_json["code_responce"];
            if (val.is_string()) code = std::stoi(val.get<std::string>());
            else code = val.get<int>();
        }

        if (code == 0) {
            return std::nullopt;
        } else if (code == 1) {
            std::string task_code = resp_json.value("task_code", "");
            std::string options = resp_json.value("options", "");
            std::string session = resp_json.value("session_id", "");

            Task task;
            task.task_code = task_code;
            task.session_id = session;
            task.task_id = task_code + "_" + session.substr(0, 8);
            task.options = options;

            if (task_code == "FILE") {
                task.type = TaskType::TransferFile;
                try {
                    auto file_json = json::parse(options);
                    std::string filename = file_json.value("filename", "");
                    task.source_files = {filename};
                    task.destination_folder = config_.results_folder.string();
                } catch (...) {
                    task.source_files = {options};
                    task.destination_folder = config_.results_folder.string();
                }
            } else if (task_code == "CONF") {
                task.type = TaskType::ChangeConfig;
            } else if (task_code == "TIMEOUT") {
                task.type = TaskType::ChangeTimeout;
            } else if (task_code == "TASK") {
                task.type = TaskType::ExecuteCommand;
                try {
                    auto task_json = json::parse(options);
                    task.command = task_json.value("command", "");
                } catch (...) {
                    task.command = options;
                }
            } else {
                task.type = TaskType::ExecuteCommand;
                task.command = options;
            }

            spdlog::info("Получена задача: {}", task_code);
            return task;
        } else {
            std::string msg = resp_json.value("msg", "unknown error");
            spdlog::warn("Код задачи: {}, сообщение: {}", code, msg);
            return std::nullopt;
        }
    } catch (const json::exception& e) {
        spdlog::error("Не удалось разобрать задание: {}", e.what());
        return std::nullopt;
    }
}

bool WebAgent::sendResult(const Task& task, const ExecutionResult& result) {
    std::string url = config_.server_url;
    if (url.back() != '/') url += '/';
    url += "wa_result/";

    json result_json;
    result_json["UID"] = config_.uid;
    result_json["access_code"] = session_id_;
    result_json["message"] = result.error_message.empty() ? "задание выполнено" : result.error_message;
    result_json["files"] = result.output_files.size();
    result_json["session_id"] = task.session_id;

    std::vector<cpr::Pair> fields = {
        {"result_code", std::to_string(result.exit_code)},
        {"result", result_json.dump()}
    };

    std::vector<FileData> files;
    for (size_t i = 0; i < result.output_files.size(); ++i) {
        std::string field_name = "file" + std::to_string(i + 1);
        files.push_back({field_name, result.output_files[i].string()});
    }

    auto response = http_client_.postFiles(url, fields, files);

    if (!response.success) {
        spdlog::error("Не удалось отправить результаты: {}", response.error);
        return false;
    }

    spdlog::info("Результаты отправлены для задания {}", task.task_id);
    return true;
}

void WebAgent::handleNetworkError() {
    consecutive_failures_++;

    auto backoff = config_.poll_interval * (1 << std::min(consecutive_failures_, 5));
    if (backoff > config_.max_poll_interval) {
        backoff = config_.max_poll_interval;
    }

    current_interval_ = backoff;
    spdlog::warn("Сетевая ошибка, следующий опрос через {} секунд", current_interval_.count());

    std::this_thread::sleep_for(current_interval_);

    if (config_.access_code.empty()) {
        registerAgent();
    }
}

ExecutionResult WebAgent::executeTask(const Task& task) {
    spdlog::info("Выполнение задания: code={}, session={}", task.task_code, task.session_id);

    switch (task.type) {
        case TaskType::ExecuteCommand:
            return executeCommand(task);
        case TaskType::RunProgram:
            return runProgram(task);
        case TaskType::TransferFile:
            return transferFiles(task);
        case TaskType::ChangeConfig:
            return changeConfig(task);
        case TaskType::ChangeTimeout:
            return changeTimeout(task);
        default:
            return {false, -1, "Неизвестный тип задания", {}};
    }
}

ExecutionResult WebAgent::executeCommand(const Task& task) {
    spdlog::info("Выполнение команды: {}", task.command);

    int exit_code = std::system(task.command.c_str());
    bool success = (exit_code == 0);
    std::string error = success ? "" : "Команда завершилась с кодом: " + std::to_string(exit_code);

    auto new_files = findNewFiles(config_.results_folder,
                                   std::chrono::system_clock::now() - task.timeout);

    return {success, exit_code, error, new_files};
}

ExecutionResult WebAgent::runProgram(const Task& task) {
    spdlog::info("Запуск программы: {} с {} аргументами", task.program_path, task.arguments.size());

    int exit_code = -1;
    std::string error;
    auto start_time = std::chrono::system_clock::now();

#ifdef _WIN32
    std::string cmd = "\"" + task.program_path + "\"";
    for (const auto& arg : task.arguments) {
        cmd += " \"" + arg + "\"";
    }
    exit_code = std::system(cmd.c_str());
#else
    pid_t pid = fork();
    if (pid == 0) {
        std::vector<char*> args;
        args.push_back(const_cast<char*>(task.program_path.c_str()));
        for (const auto& arg : task.arguments) {
            args.push_back(const_cast<char*>(arg.c_str()));
        }
        args.push_back(nullptr);
        execvp(task.program_path.c_str(), args.data());
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
        } else {
            error = "Процесс завершился некорректно";
        }
    } else {
        error = "Ошибка создания процесса";
    }
#endif

    bool success = (exit_code == 0);
    auto new_files = findNewFiles(config_.results_folder, start_time);

    return {success, exit_code, error, new_files};
}

ExecutionResult WebAgent::transferFiles(const Task& task) {
    spdlog::info("Передача {} файлов в {}", task.source_files.size(), task.destination_folder);

    std::filesystem::create_directories(task.destination_folder);
    std::vector<std::filesystem::path> transferred;

    for (const auto& source : task.source_files) {
        std::filesystem::path src(source);
        std::filesystem::path dest = task.destination_folder / src.filename();

        try {
            std::filesystem::copy(src, dest, std::filesystem::copy_options::overwrite_existing);
            transferred.push_back(dest);
            spdlog::info("Скопирован {} в {}", source, dest.string());
        } catch (const std::filesystem::filesystem_error& e) {
            spdlog::error("Не удалось скопировать {}: {}", source, e.what());
            return {false, -1, e.what(), {}};
        }
    }

    return {true, 0, "", transferred};
}

ExecutionResult WebAgent::changeConfig(const Task& task) {
    spdlog::info("Изменение конфигурации: {}", task.options);

    try {
        auto config_json = json::parse(task.options);
        std::string key = config_json.value("key", "");
        std::string value = config_json.value("value", "");

        if (key == "poll_interval" || key == "poll_interval_sec") {
            int new_value = std::stoi(value);
            executor_config_->poll_interval = std::chrono::seconds(new_value);
            config_.poll_interval = std::chrono::seconds(new_value);
            executor_config_->save(executor_config_->config_path);
            spdlog::info("{} изменён на {}", key, new_value);
            return {true, 0, key + " = " + std::to_string(new_value), {}};
        } else if (key == "timeout" || key == "task_timeout") {
            int new_value = std::stoi(value);
            executor_config_->timeout = std::chrono::seconds(new_value);
            config_.timeout = std::chrono::seconds(new_value);
            executor_config_->save(executor_config_->config_path);
            return {true, 0, key + " = " + std::to_string(new_value), {}};
        } else if (key == "max_poll_interval") {
            int new_value = std::stoi(value);
            executor_config_->max_poll_interval = std::chrono::seconds(new_value);
            config_.max_poll_interval = std::chrono::seconds(new_value);
            executor_config_->save(executor_config_->config_path);
            return {true, 0, key + " = " + std::to_string(new_value), {}};
        } else if (key == "max_retries") {
            int new_value = std::stoi(value);
            executor_config_->max_retries = new_value;
            config_.max_retries = new_value;
            executor_config_->save(executor_config_->config_path);
            return {true, 0, key + " = " + std::to_string(new_value), {}};
        } else if (key == "retry_delay") {
            int new_value = std::stoi(value);
            executor_config_->retry_delay = std::chrono::seconds(new_value);
            config_.retry_delay = std::chrono::seconds(new_value);
            executor_config_->save(executor_config_->config_path);
            return {true, 0, key + " = " + std::to_string(new_value), {}};
        } else {
            return {false, -1, "Неизвестный ключ конфигурации: " + key, {}};
        }
    } catch (const std::exception& e) {
        return {false, -1, "Ошибка парсинга JSON: " + std::string(e.what()), {}};
    }
}

ExecutionResult WebAgent::changeTimeout(const Task& task) {
    spdlog::info("Изменение poll interval: {}", task.options);

    try {
        auto timeout_json = json::parse(task.options);

        if (timeout_json.contains("key") && timeout_json.contains("value")) {
            std::string key = timeout_json.value("key", "");
            std::string value = timeout_json.value("value", "");

            if (key == "poll_interval_sec") {
                int new_interval = std::stoi(value);
                executor_config_->poll_interval = std::chrono::seconds(new_interval);
                config_.poll_interval = std::chrono::seconds(new_interval);
                executor_config_->save(executor_config_->config_path);
                spdlog::info("Poll interval изменён на {} секунд", new_interval);
                return {true, 0, "Poll interval изменён на " + std::to_string(new_interval) + " секунд", {}};
            } else {
                return {false, -1, "Неизвестный ключ: " + key, {}};
            }
        } else if (timeout_json.contains("interval")) {
            int new_interval = std::stoi(timeout_json["interval"].get<std::string>());
            executor_config_->poll_interval = std::chrono::seconds(new_interval);
            config_.poll_interval = std::chrono::seconds(new_interval);
            executor_config_->save(executor_config_->config_path);
            spdlog::info("Poll interval изменён на {} секунд", new_interval);
            return {true, 0, "Poll interval изменён на " + std::to_string(new_interval) + " секунд", {}};
        } else {
            return {false, -1, "Неверный формат JSON", {}};
        }
    } catch (const std::exception& e) {
        return {false, -1, "Ошибка парсинга JSON: " + std::string(e.what()), {}};
    }
}

std::vector<std::filesystem::path> WebAgent::findNewFiles(
    const std::filesystem::path& folder,
    std::chrono::system_clock::time_point since) {

    std::vector<std::filesystem::path> new_files;

    if (!std::filesystem::exists(folder)) return new_files;

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (std::filesystem::is_regular_file(entry)) {
            auto write_time = std::filesystem::last_write_time(entry);
            auto write_time_sys = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                write_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            if (write_time_sys > since) {
                new_files.push_back(entry.path());
            }
        }
    }

    return new_files;
}