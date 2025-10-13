#include "core/config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace nerd {

Config* Config::instance_ = nullptr;

Config::Config() {
    load_defaults();
}

Config& Config::instance() {
    if (!instance_) {
        instance_ = new Config();
    }
    return *instance_;
}

void Config::load_defaults() {
    // Network defaults
    network_.interface = "eth0";
    network_.circulation_rate = 10;
    network_.max_packet_age = 30;
    network_.heartbeat_interval = 5;
    network_.port = 31337;
    network_.auto_discovery = true;
    network_.enable_encryption = false;
    network_.max_packet_size = 1400;
    network_.buffer_size = 65536;
    network_.worker_threads = 4;
    
    // Security defaults
    security_.require_auth = false;
    security_.enable_tls = false;
    
    // Storage defaults
    storage_.enable_persistence = false;
    storage_.data_dir = "/var/lib/nerd";
    storage_.max_cache_size = 100;
    storage_.enable_compression = true;
    
    // Logging defaults
    logging_.log_file = "/var/log/nerd.log";
    logging_.log_level = LogLevel::INFO;
    logging_.log_to_console = true;
    logging_.log_to_file = true;
    logging_.max_log_size = 100;
    logging_.max_log_files = 10;
}

bool Config::load(const std::string& config_file) {
    config_file_ = config_file;
    
    std::ifstream file(config_file);
    if (!file.is_open()) {
        // Config file not found, use defaults
        LOG_INFO("Config file not found, using defaults: " + config_file);
        return true;
    }
    
    std::string line;
    std::string current_section;
    int line_number = 0;
    
    while (std::getline(file, line)) {
        line_number++;
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Check for section header
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key-value pair
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = line.substr(0, equals_pos);
            std::string value = line.substr(equals_pos + 1);
            
            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (!parse_value(current_section, key, value)) {
                LOG_WARNING("Invalid config at line " + std::to_string(line_number) + ": " + line);
            }
        }
    }
    
    file.close();
    
    if (!validate()) {
        LOG_ERROR("Configuration validation failed");
        return false;
    }
    
    LOG_INFO("Configuration loaded from: " + config_file);
    return true;
}

bool Config::save(const std::string& config_file) {
    std::string file_path = config_file.empty() ? config_file_ : config_file;
    
    std::ofstream file(file_path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file for writing: " + file_path);
        return false;
    }
    
    file << "# NERD Configuration File\n";
    file << "# Generated automatically\n\n";
    
    // Network section
    file << "[network]\n";
    file << "interface = " << network_.interface << "\n";
    file << "circulation_rate = " << network_.circulation_rate << "\n";
    file << "max_packet_age = " << network_.max_packet_age << "\n";
    file << "heartbeat_interval = " << network_.heartbeat_interval << "\n";
    file << "port = " << network_.port << "\n";
    file << "auto_discovery = " << (network_.auto_discovery ? "true" : "false") << "\n";
    file << "enable_encryption = " << (network_.enable_encryption ? "true" : "false") << "\n";
    file << "max_packet_size = " << network_.max_packet_size << "\n";
    file << "buffer_size = " << network_.buffer_size << "\n";
    file << "worker_threads = " << network_.worker_threads << "\n\n";
    
    // Security section
    file << "[security]\n";
    file << "require_auth = " << (security_.require_auth ? "true" : "false") << "\n";
    if (!security_.auth_key.empty()) {
        file << "auth_key = " << security_.auth_key << "\n";
    }
    file << "enable_tls = " << (security_.enable_tls ? "true" : "false") << "\n";
    if (!security_.cert_file.empty()) {
        file << "cert_file = " << security_.cert_file << "\n";
    }
    if (!security_.key_file.empty()) {
        file << "key_file = " << security_.key_file << "\n";
    }
    file << "\n";
    
    // Storage section
    file << "[storage]\n";
    file << "enable_persistence = " << (storage_.enable_persistence ? "true" : "false") << "\n";
    file << "data_dir = " << storage_.data_dir << "\n";
    file << "max_cache_size = " << storage_.max_cache_size << "\n";
    file << "enable_compression = " << (storage_.enable_compression ? "true" : "false") << "\n\n";
    
    // Logging section
    file << "[logging]\n";
    file << "log_file = " << logging_.log_file << "\n";
    file << "log_level = " << static_cast<int>(logging_.log_level) << "\n";
    file << "log_to_console = " << (logging_.log_to_console ? "true" : "false") << "\n";
    file << "log_to_file = " << (logging_.log_to_file ? "true" : "false") << "\n";
    file << "max_log_size = " << logging_.max_log_size << "\n";
    file << "max_log_files = " << logging_.max_log_files << "\n";
    
    file.close();
    
    LOG_INFO("Configuration saved to: " + file_path);
    return true;
}

bool Config::parse_value(const std::string& section, const std::string& key, const std::string& value) {
    if (section == "network") {
        if (key == "interface") {
            network_.interface = value;
        } else if (key == "circulation_rate") {
            network_.circulation_rate = std::stoul(value);
        } else if (key == "max_packet_age") {
            network_.max_packet_age = std::stoul(value);
        } else if (key == "heartbeat_interval") {
            network_.heartbeat_interval = std::stoul(value);
        } else if (key == "port") {
            network_.port = std::stoul(value);
        } else if (key == "auto_discovery") {
            network_.auto_discovery = (value == "true" || value == "1");
        } else if (key == "enable_encryption") {
            network_.enable_encryption = (value == "true" || value == "1");
        } else if (key == "max_packet_size") {
            network_.max_packet_size = std::stoul(value);
        } else if (key == "buffer_size") {
            network_.buffer_size = std::stoul(value);
        } else if (key == "worker_threads") {
            network_.worker_threads = std::stoul(value);
        } else {
            return false;
        }
    } else if (section == "security") {
        if (key == "require_auth") {
            security_.require_auth = (value == "true" || value == "1");
        } else if (key == "auth_key") {
            security_.auth_key = value;
        } else if (key == "enable_tls") {
            security_.enable_tls = (value == "true" || value == "1");
        } else if (key == "cert_file") {
            security_.cert_file = value;
        } else if (key == "key_file") {
            security_.key_file = value;
        } else if (key == "allowed_ip") {
            security_.allowed_ips.push_back(value);
        } else if (key == "blocked_ip") {
            security_.blocked_ips.push_back(value);
        } else {
            return false;
        }
    } else if (section == "storage") {
        if (key == "enable_persistence") {
            storage_.enable_persistence = (value == "true" || value == "1");
        } else if (key == "data_dir") {
            storage_.data_dir = value;
        } else if (key == "max_cache_size") {
            storage_.max_cache_size = std::stoul(value);
        } else if (key == "enable_compression") {
            storage_.enable_compression = (value == "true" || value == "1");
        } else {
            return false;
        }
    } else if (section == "logging") {
        if (key == "log_file") {
            logging_.log_file = value;
        } else if (key == "log_level") {
            logging_.log_level = static_cast<LogLevel>(std::stoi(value));
        } else if (key == "log_to_console") {
            logging_.log_to_console = (value == "true" || value == "1");
        } else if (key == "log_to_file") {
            logging_.log_to_file = (value == "true" || value == "1");
        } else if (key == "max_log_size") {
            logging_.max_log_size = std::stoul(value);
        } else if (key == "max_log_files") {
            logging_.max_log_files = std::stoul(value);
        } else {
            return false;
        }
    }
    
    return true;
}

bool Config::validate() const {
    // Validate network configuration
    if (network_.interface.empty()) {
        LOG_ERROR("Network interface not specified");
        return false;
    }
    
    if (network_.circulation_rate == 0 || network_.circulation_rate > 1000) {
        LOG_ERROR("Invalid circulation rate: " + std::to_string(network_.circulation_rate));
        return false;
    }
    
    if (network_.max_packet_size < 64 || network_.max_packet_size > 65535) {
        LOG_ERROR("Invalid max packet size: " + std::to_string(network_.max_packet_size));
        return false;
    }
    
    // Validate security configuration
    if (security_.require_auth && security_.auth_key.empty()) {
        LOG_ERROR("Authentication required but no auth key specified");
        return false;
    }
    
    if (security_.enable_tls) {
        if (security_.cert_file.empty() || security_.key_file.empty()) {
            LOG_ERROR("TLS enabled but certificate or key file not specified");
            return false;
        }
    }
    
    // Validate storage configuration
    if (storage_.enable_persistence && storage_.data_dir.empty()) {
        LOG_ERROR("Persistence enabled but no data directory specified");
        return false;
    }
    
    // Validate logging configuration
    if (logging_.log_to_file && logging_.log_file.empty()) {
        LOG_ERROR("File logging enabled but no log file specified");
        return false;
    }
    
    return true;
}

} // namespace nerd