#include "logger.h"
#include "agent.h"
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <spdlog/spdlog.h>

Logger::Logger(const std::filesystem::path& log_file) : log_file_(log_file) {
    spdlog::info("Журнал инициализирован, файл: {}", log_file.string());
}

void Logger::logTask(const Task& task, const ExecutionResult& result) {
    std::string message = "Задание " + task.task_id + " выполнено. ";
    message += "Успех: " + std::string(result.success ? "да" : "нет");
    message += ", Код возврата: " + std::to_string(result.exit_code);
    message += ", Выходные файлы: " + std::to_string(result.output_files.size());

    write("ИНФО", message);
}

void Logger::logError(const std::string& task_id, const std::string& error) {
    write("ОШИБКА", "Задание " + task_id + " не выполнено: " + error);
}

void Logger::logInfo(const std::string& message) {
    write("ИНФО", message);
}

void Logger::logWarning(const std::string& message) {
    write("ПРЕДУПРЕЖДЕНИЕ", message);
}

void Logger::write(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ofstream file(log_file_, std::ios::app);
    if (file.is_open()) {
        file << "[" << getTimestamp() << "] [" << level << "] " << message << std::endl;
    }

    if (level == "ОШИБКА") {
        spdlog::error(message);
    } else if (level == "ПРЕДУПРЕЖДЕНИЕ") {
        spdlog::warn(message);
    } else {
        spdlog::info(message);
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}