#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace nerd {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
private:
    static std::unique_ptr<Logger> instance_;
    static std::mutex mutex_;
    
    std::ofstream log_file_;
    LogLevel min_level_;
    bool console_output_;
    bool file_output_;
    std::mutex write_mutex_;
    
    Logger();
    
public:
    static Logger& instance();
    
    void initialize(const std::string& log_file = "/var/log/nerd.log",
                   LogLevel min_level = LogLevel::INFO,
                   bool console = true,
                   bool file = true);
    
    void log(LogLevel level, const std::string& message, const std::string& module = "");
    
    void debug(const std::string& message, const std::string& module = "");
    void info(const std::string& message, const std::string& module = "");
    void warning(const std::string& message, const std::string& module = "");
    void error(const std::string& message, const std::string& module = "");
    void critical(const std::string& message, const std::string& module = "");
    
    void set_level(LogLevel level) { min_level_ = level; }
    void enable_console(bool enable) { console_output_ = enable; }
    void enable_file(bool enable) { file_output_ = enable; }
    
private:
    std::string level_to_string(LogLevel level) const;
    std::string get_timestamp() const;
};

// Convenience macros
#define LOG_DEBUG(msg) nerd::Logger::instance().debug(msg, __func__)
#define LOG_INFO(msg) nerd::Logger::instance().info(msg, __func__)
#define LOG_WARNING(msg) nerd::Logger::instance().warning(msg, __func__)
#define LOG_ERROR(msg) nerd::Logger::instance().error(msg, __func__)
#define LOG_CRITICAL(msg) nerd::Logger::instance().critical(msg, __func__)

} // namespace nerd