#include "core/logger.h"
#include <iostream>
#include <ctime>

namespace nerd {

std::unique_ptr<Logger> Logger::instance_ = nullptr;
std::mutex Logger::mutex_;

Logger::Logger() : min_level_(LogLevel::INFO), console_output_(true), file_output_(false) {}

Logger& Logger::instance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    return *instance_;
}

void Logger::initialize(const std::string& log_file, LogLevel min_level, bool console, bool file) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    min_level_ = min_level;
    console_output_ = console;
    file_output_ = file;
    
    if (file_output_ && !log_file.empty()) {
        if (log_file_.is_open()) {
            log_file_.close();
        }
        log_file_.open(log_file, std::ios::app);
        if (!log_file_) {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
            file_output_ = false;
        }
    }
}

void Logger::log(LogLevel level, const std::string& message, const std::string& module) {
    if (level < min_level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(write_mutex_);
    
    std::string timestamp = get_timestamp();
    std::string level_str = level_to_string(level);
    
    std::ostringstream log_entry;
    log_entry << "[" << timestamp << "] [" << level_str << "]";
    if (!module.empty()) {
        log_entry << " [" << module << "]";
    }
    log_entry << " " << message;
    
    if (console_output_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << log_entry.str() << std::endl;
        } else {
            std::cout << log_entry.str() << std::endl;
        }
    }
    
    if (file_output_ && log_file_.is_open()) {
        log_file_ << log_entry.str() << std::endl;
        log_file_.flush();
    }
}

void Logger::debug(const std::string& message, const std::string& module) {
    log(LogLevel::DEBUG, message, module);
}

void Logger::info(const std::string& message, const std::string& module) {
    log(LogLevel::INFO, message, module);
}

void Logger::warning(const std::string& message, const std::string& module) {
    log(LogLevel::WARNING, message, module);
}

void Logger::error(const std::string& message, const std::string& module) {
    log(LogLevel::ERROR, message, module);
}

void Logger::critical(const std::string& message, const std::string& module) {
    log(LogLevel::CRITICAL, message, module);
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARN";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default:                 return "UNKNOWN";
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm;
    localtime_r(&time_t, &tm);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

} // namespace nerd