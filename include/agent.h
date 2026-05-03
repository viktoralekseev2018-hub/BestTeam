#pragma once

#include "config.h"
#include "http_client.h"
#include <atomic>
#include <memory>
#include <optional>
#include <thread>

// Предварительное объявление
class Logger;
struct ExecutionResult;

// Структура задачи
enum class TaskType {
    None,
    ExecuteCommand,
    RunProgram,
    TransferFile,
    ChangeConfig,
    ChangeTimeout
};

struct Task {
    TaskType type = TaskType::None;
    std::string task_code;
    std::string task_id;
    std::string session_id;
    std::string command;
    std::string program_path;
    std::vector<std::string> arguments;
    std::vector<std::string> source_files;
    std::string destination_folder;
    std::string options;
    std::chrono::seconds timeout{300};

    bool isValid() const;
    static Task fromJson(const std::string& json);
};

// Результат выполнения
struct ExecutionResult {
    bool success;
    int exit_code;
    std::string error_message;
    std::vector<std::filesystem::path> output_files;
};

// Основной класс агента
class WebAgent {
public:
    explicit WebAgent(const Config& config);
    ~WebAgent();

    void start();
    void stop();
    bool isRunning() const { return running_; }

private:
    Config config_;
    HttpClient http_client_;
    std::unique_ptr<class Logger> logger_;
    std::shared_ptr<Config> executor_config_;

    std::atomic<bool> running_{false};
    std::thread worker_thread_;
    std::string session_id_;
    std::chrono::seconds current_interval_;
    int consecutive_failures_{0};

    void workerLoop();
    bool registerAgent();
    std::optional<Task> fetchTask();
    bool sendResult(const Task& task, const ExecutionResult& result);
    void handleNetworkError();

    // Выполнение задач
    ExecutionResult executeTask(const Task& task);
    ExecutionResult executeCommand(const Task& task);
    ExecutionResult runProgram(const Task& task);
    ExecutionResult transferFiles(const Task& task);
    ExecutionResult changeConfig(const Task& task);
    ExecutionResult changeTimeout(const Task& task);

    static std::vector<std::filesystem::path> findNewFiles(
        const std::filesystem::path& folder,
        std::chrono::system_clock::time_point since);
};