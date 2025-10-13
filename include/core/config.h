#pragma once

#include <string>
#include <map>
#include <vector>
#include "core/logger.h"

namespace nerd {

struct NetworkConfig {
    std::string interface = "eth0";
    uint32_t circulation_rate = 10;  // packets per second
    uint32_t max_packet_age = 30;    // seconds
    uint32_t heartbeat_interval = 5; // seconds
    uint16_t port = 31337;
    bool auto_discovery = true;
    bool enable_encryption = false;
    
    // Performance tuning
    uint32_t max_packet_size = 1400;
    uint32_t buffer_size = 65536;
    uint32_t worker_threads = 4;
};

struct SecurityConfig {
    bool require_auth = false;
    std::string auth_key;
    bool enable_tls = false;
    std::string cert_file;
    std::string key_file;
    
    // Access control
    std::vector<std::string> allowed_ips;
    std::vector<std::string> blocked_ips;
};

struct StorageConfig {
    bool enable_persistence = false;
    std::string data_dir = "/var/lib/nerd";
    uint32_t max_cache_size = 100; // MB
    bool enable_compression = true;
};

struct LoggingConfig {
    std::string log_file = "/var/log/nerd.log";
    LogLevel log_level = LogLevel::INFO;
    bool log_to_console = true;
    bool log_to_file = true;
    uint32_t max_log_size = 100; // MB
    uint32_t max_log_files = 10;
};

class Config {
private:
    static Config* instance_;
    
    NetworkConfig network_;
    SecurityConfig security_;
    StorageConfig storage_;
    LoggingConfig logging_;
    
    std::string config_file_;
    
public:
    static Config& instance();
    
    bool load(const std::string& config_file = "/etc/nerd/nerd.conf");
    bool save(const std::string& config_file = "");
    
    // Getters
    const NetworkConfig& network() const { return network_; }
    const SecurityConfig& security() const { return security_; }
    const StorageConfig& storage() const { return storage_; }
    const LoggingConfig& logging() const { return logging_; }
    
    // Setters
    NetworkConfig& network() { return network_; }
    SecurityConfig& security() { return security_; }
    StorageConfig& storage() { return storage_; }
    LoggingConfig& logging() { return logging_; }
    
    // Validation
    bool validate() const;
    
    // Default configuration
    void load_defaults();
    
private:
    Config();
    bool parse_line(const std::string& line);
    bool parse_value(const std::string& section, const std::string& key, const std::string& value);
};

} // namespace nerd