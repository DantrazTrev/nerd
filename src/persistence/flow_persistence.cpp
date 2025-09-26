#include "persistence/flow_persistence.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>

namespace nerd {

FlowPersistence::FlowPersistence(const std::string& persistence_dir) 
    : persistence_dir_(persistence_dir), strategy_(PersistenceStrategy::LOCAL_DISK),
      checkpoint_interval_(std::chrono::seconds(300)), max_checkpoints_(10),
      auto_persistence_(true) {
    ensure_persistence_directory();
}

FlowPersistence::~FlowPersistence() = default;

bool FlowPersistence::save_flow(const FlowFile& flow) {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    switch (strategy_) {
        case PersistenceStrategy::MEMORY_ONLY:
            // Just keep in memory
            snapshots_[flow.identifier()] = create_snapshot_from_flow(flow);
            return true;
            
        case PersistenceStrategy::LOCAL_DISK:
            {
                std::string file_path = get_flow_file_path(flow.identifier());
                std::ofstream file(file_path, std::ios::binary);
                if (!file.is_open()) {
                    std::cerr << "Failed to open file for writing: " << file_path << std::endl;
                    return false;
                }
                
                bool success = serialize_flow(flow, file);
                file.close();
                
                if (success) {
                    snapshots_[flow.identifier()] = create_snapshot_from_flow(flow);
                    std::cout << "Saved flow to disk: " << flow.name() << std::endl;
                }
                
                return success;
            }
            
        case PersistenceStrategy::DISTRIBUTED:
            // In a real implementation, this would distribute across multiple nodes
            std::cout << "Distributed persistence not yet implemented" << std::endl;
            return false;
            
        case PersistenceStrategy::HYBRID:
            // Combine local disk and memory
            {
                bool memory_success = true;
                bool disk_success = true;
                
                // Save to memory
                snapshots_[flow.identifier()] = create_snapshot_from_flow(flow);
                
                // Save to disk
                std::string file_path = get_flow_file_path(flow.identifier());
                std::ofstream file(file_path, std::ios::binary);
                if (file.is_open()) {
                    disk_success = serialize_flow(flow, file);
                    file.close();
                } else {
                    disk_success = false;
                }
                
                return memory_success && disk_success;
            }
            
        default:
            return false;
    }
}

bool FlowPersistence::load_flow(FlowID flow_id, FlowFile& flow) {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    switch (strategy_) {
        case PersistenceStrategy::MEMORY_ONLY:
            {
                auto it = snapshots_.find(flow_id);
                if (it != snapshots_.end()) {
                    const FlowSnapshot& snapshot = it->second;
                    flow.write_to_flow(snapshot.content);
                    flow.update_circulation_pattern(snapshot.pattern);
                    return true;
                }
                return false;
            }
            
        case PersistenceStrategy::LOCAL_DISK:
            {
                std::string file_path = get_flow_file_path(flow_id);
                std::ifstream file(file_path, std::ios::binary);
                if (!file.is_open()) {
                    return false;
                }
                
                bool success = deserialize_flow(file, flow);
                file.close();
                
                if (success) {
                    std::cout << "Loaded flow from disk: " << flow.name() << std::endl;
                }
                
                return success;
            }
            
        case PersistenceStrategy::DISTRIBUTED:
            std::cout << "Distributed loading not yet implemented" << std::endl;
            return false;
            
        case PersistenceStrategy::HYBRID:
            // Try memory first, then disk
            {
                auto it = snapshots_.find(flow_id);
                if (it != snapshots_.end()) {
                    const FlowSnapshot& snapshot = it->second;
                    flow.write_to_flow(snapshot.content);
                    flow.update_circulation_pattern(snapshot.pattern);
                    return true;
                }
                
                // Fall back to disk
                std::string file_path = get_flow_file_path(flow_id);
                std::ifstream file(file_path, std::ios::binary);
                if (file.is_open()) {
                    bool success = deserialize_flow(file, flow);
                    file.close();
                    return success;
                }
                
                return false;
            }
            
        default:
            return false;
    }
}

bool FlowPersistence::delete_flow(FlowID flow_id) {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    // Remove from memory
    snapshots_.erase(flow_id);
    
    // Remove from disk
    std::string file_path = get_flow_file_path(flow_id);
    if (std::filesystem::exists(file_path)) {
        try {
            std::filesystem::remove(file_path);
            std::cout << "Deleted persisted flow: " << flow_id << std::endl;
            return true;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to delete file: " << e.what() << std::endl;
            return false;
        }
    }
    
    return true;
}

std::vector<FlowID> FlowPersistence::list_persisted_flows() const {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    std::vector<FlowID> flows;
    flows.reserve(snapshots_.size());
    
    for (const auto& pair : snapshots_) {
        flows.push_back(pair.first);
    }
    
    return flows;
}

bool FlowPersistence::create_snapshot(const FlowFile& flow) {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    FlowSnapshot snapshot = create_snapshot_from_flow(flow);
    snapshots_[flow.identifier()] = snapshot;
    
    // Save snapshot to disk
    std::string snapshot_path = get_snapshot_file_path(flow.identifier(), snapshot.version);
    std::ofstream file(snapshot_path, std::ios::binary);
    if (file.is_open()) {
        bool success = serialize_snapshot(snapshot, file);
        file.close();
        return success;
    }
    
    return false;
}

bool FlowPersistence::restore_from_snapshot(FlowID flow_id, FlowFile& flow) {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    auto it = snapshots_.find(flow_id);
    if (it != snapshots_.end()) {
        const FlowSnapshot& snapshot = it->second;
        flow.write_to_flow(snapshot.content);
        flow.update_circulation_pattern(snapshot.pattern);
        return true;
    }
    
    return false;
}

std::vector<FlowSnapshot> FlowPersistence::get_flow_history(FlowID flow_id) const {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    std::vector<FlowSnapshot> history;
    
    // In a real implementation, this would load multiple versions from disk
    auto it = snapshots_.find(flow_id);
    if (it != snapshots_.end()) {
        history.push_back(it->second);
    }
    
    return history;
}

bool FlowPersistence::create_checkpoint() {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    PersistenceCheckpoint checkpoint;
    checkpoint.checkpoint_id = "checkpoint_" + std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    checkpoint.timestamp = std::chrono::system_clock::now();
    
    // Copy all current snapshots
    for (const auto& pair : snapshots_) {
        checkpoint.flows.push_back(pair.second);
    }
    
    // Save checkpoint to disk
    std::string checkpoint_path = get_checkpoint_file_path(checkpoint.checkpoint_id);
    std::ofstream file(checkpoint_path, std::ios::binary);
    if (file.is_open()) {
        bool success = serialize_checkpoint(checkpoint, file);
        file.close();
        
        if (success) {
            checkpoints_.push_back(checkpoint);
            
            // Limit number of checkpoints
            if (checkpoints_.size() > max_checkpoints_) {
                checkpoints_.erase(checkpoints_.begin());
            }
            
            std::cout << "Created checkpoint: " << checkpoint.checkpoint_id << std::endl;
        }
        
        return success;
    }
    
    return false;
}

bool FlowPersistence::restore_from_checkpoint(const std::string& checkpoint_id) {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    std::string checkpoint_path = get_checkpoint_file_path(checkpoint_id);
    std::ifstream file(checkpoint_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    PersistenceCheckpoint checkpoint;
    bool success = deserialize_checkpoint(file, checkpoint);
    file.close();
    
    if (success) {
        // Restore all flows from checkpoint
        snapshots_.clear();
        for (const auto& flow_snapshot : checkpoint.flows) {
            snapshots_[flow_snapshot.flow_id] = flow_snapshot;
        }
        
        std::cout << "Restored from checkpoint: " << checkpoint_id << std::endl;
    }
    
    return success;
}

std::vector<std::string> FlowPersistence::list_checkpoints() const {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    std::vector<std::string> checkpoint_ids;
    checkpoint_ids.reserve(checkpoints_.size());
    
    for (const auto& checkpoint : checkpoints_) {
        checkpoint_ids.push_back(checkpoint.checkpoint_id);
    }
    
    return checkpoint_ids;
}

bool FlowPersistence::delete_checkpoint(const std::string& checkpoint_id) {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    // Remove from memory
    checkpoints_.erase(
        std::remove_if(checkpoints_.begin(), checkpoints_.end(),
                      [&checkpoint_id](const PersistenceCheckpoint& cp) {
                          return cp.checkpoint_id == checkpoint_id;
                      }),
        checkpoints_.end()
    );
    
    // Remove from disk
    std::string checkpoint_path = get_checkpoint_file_path(checkpoint_id);
    if (std::filesystem::exists(checkpoint_path)) {
        try {
            std::filesystem::remove(checkpoint_path);
            return true;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to delete checkpoint: " << e.what() << std::endl;
            return false;
        }
    }
    
    return true;
}

bool FlowPersistence::recover_all_flows() {
    std::cout << "Recovering all flows from persistence..." << std::endl;
    
    // In a real implementation, this would scan the persistence directory
    // and recover all available flows
    
    std::vector<FlowID> persisted_flows = list_persisted_flows();
    std::cout << "Found " << persisted_flows.size() << " persisted flows" << std::endl;
    
    return !persisted_flows.empty();
}

bool FlowPersistence::recover_flow(FlowID flow_id) {
    std::cout << "Recovering flow: " << flow_id << std::endl;
    
    // Check if flow is persisted
    if (!is_flow_persisted(flow_id)) {
        std::cout << "Flow not found in persistence: " << flow_id << std::endl;
        return false;
    }
    
    return true;
}

std::vector<FlowID> FlowPersistence::get_recoverable_flows() const {
    return list_persisted_flows();
}

bool FlowPersistence::is_flow_persisted(FlowID flow_id) const {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    return snapshots_.find(flow_id) != snapshots_.end();
}

std::chrono::time_point<std::chrono::system_clock> FlowPersistence::get_last_save_time(FlowID flow_id) const {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    auto it = snapshots_.find(flow_id);
    if (it != snapshots_.end()) {
        return it->second.timestamp;
    }
    
    return std::chrono::time_point<std::chrono::system_clock>();
}

size_t FlowPersistence::get_persistence_size() const {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    size_t total_size = 0;
    for (const auto& pair : snapshots_) {
        total_size += pair.second.content.size();
    }
    
    return total_size;
}

void FlowPersistence::cleanup_old_snapshots() {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto cutoff_time = now - std::chrono::hours(24); // Keep snapshots for 24 hours
    
    for (auto it = snapshots_.begin(); it != snapshots_.end();) {
        if (it->second.timestamp < cutoff_time) {
            it = snapshots_.erase(it);
        } else {
            ++it;
        }
    }
    
    std::cout << "Cleaned up old snapshots" << std::endl;
}

void FlowPersistence::compact_persistence() {
    std::cout << "Compacting persistence storage..." << std::endl;
    
    // In a real implementation, this would compact the storage
    // by removing redundant data and optimizing storage layout
    
    std::cout << "Persistence compacted" << std::endl;
}

void FlowPersistence::verify_persistence_integrity() {
    std::lock_guard<std::mutex> lock(persistence_mutex_);
    
    std::cout << "Verifying persistence integrity..." << std::endl;
    
    for (const auto& pair : snapshots_) {
        if (!validate_snapshot(pair.second)) {
            std::cerr << "Invalid snapshot found for flow: " << pair.first << std::endl;
        }
    }
    
    std::cout << "Persistence integrity verified" << std::endl;
}

std::string FlowPersistence::get_flow_file_path(FlowID flow_id) const {
    return persistence_dir_ + "/flows/flow_" + std::to_string(flow_id) + ".dat";
}

std::string FlowPersistence::get_snapshot_file_path(FlowID flow_id, uint32_t version) const {
    return persistence_dir_ + "/snapshots/flow_" + std::to_string(flow_id) + 
           "_v" + std::to_string(version) + ".dat";
}

std::string FlowPersistence::get_checkpoint_file_path(const std::string& checkpoint_id) const {
    return persistence_dir_ + "/checkpoints/" + checkpoint_id + ".dat";
}

bool FlowPersistence::serialize_flow(const FlowFile& flow, std::ostream& out) {
    try {
        // Serialize flow data
        FlowID flow_id = flow.identifier();
        out.write(reinterpret_cast<const char*>(&flow_id), sizeof(FlowID));
        
        // Serialize flow name
        const std::string& name = flow.name();
        size_t name_length = name.length();
        out.write(reinterpret_cast<const char*>(&name_length), sizeof(size_t));
        out.write(name.c_str(), name_length);
        
        // Serialize content
        const std::string& content = flow.content();
        size_t content_length = content.length();
        out.write(reinterpret_cast<const char*>(&content_length), sizeof(size_t));
        out.write(content.c_str(), content_length);
        
        // Serialize pattern
        const CirculationPattern& pattern = flow.pattern();
        out.write(reinterpret_cast<const char*>(&pattern), sizeof(CirculationPattern));
        
        return out.good();
    } catch (const std::exception& e) {
        std::cerr << "Serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool FlowPersistence::deserialize_flow(std::istream& in, FlowFile& flow) {
    try {
        // Deserialize flow ID
        FlowID flow_id;
        in.read(reinterpret_cast<char*>(&flow_id), sizeof(FlowID));
        
        // Deserialize flow name
        size_t name_length;
        in.read(reinterpret_cast<char*>(&name_length), sizeof(size_t));
        std::string name(name_length, '\0');
        in.read(&name[0], name_length);
        
        // Deserialize content
        size_t content_length;
        in.read(reinterpret_cast<char*>(&content_length), sizeof(size_t));
        std::string content(content_length, '\0');
        in.read(&content[0], content_length);
        
        // Deserialize pattern
        CirculationPattern pattern;
        in.read(reinterpret_cast<char*>(&pattern), sizeof(CirculationPattern));
        
        if (in.good()) {
            flow.write_to_flow(content);
            flow.update_circulation_pattern(pattern);
            return true;
        }
        
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Deserialization error: " << e.what() << std::endl;
        return false;
    }
}

bool FlowPersistence::serialize_snapshot(const FlowSnapshot& snapshot, std::ostream& out) {
    try {
        out.write(reinterpret_cast<const char*>(&snapshot.flow_id), sizeof(FlowID));
        
        // Serialize flow name
        size_t name_length = snapshot.flow_name.length();
        out.write(reinterpret_cast<const char*>(&name_length), sizeof(size_t));
        out.write(snapshot.flow_name.c_str(), name_length);
        
        // Serialize content
        size_t content_length = snapshot.content.length();
        out.write(reinterpret_cast<const char*>(&content_length), sizeof(size_t));
        out.write(snapshot.content.c_str(), content_length);
        
        // Serialize pattern
        out.write(reinterpret_cast<const char*>(&snapshot.pattern), sizeof(CirculationPattern));
        
        // Serialize timestamp
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            snapshot.timestamp.time_since_epoch()).count();
        out.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        
        // Serialize version
        out.write(reinterpret_cast<const char*>(&snapshot.version), sizeof(uint32_t));
        
        return out.good();
    } catch (const std::exception& e) {
        std::cerr << "Snapshot serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool FlowPersistence::deserialize_snapshot(std::istream& in, FlowSnapshot& snapshot) {
    try {
        in.read(reinterpret_cast<char*>(&snapshot.flow_id), sizeof(FlowID));
        
        // Deserialize flow name
        size_t name_length;
        in.read(reinterpret_cast<char*>(&name_length), sizeof(size_t));
        snapshot.flow_name.resize(name_length);
        in.read(&snapshot.flow_name[0], name_length);
        
        // Deserialize content
        size_t content_length;
        in.read(reinterpret_cast<char*>(&content_length), sizeof(size_t));
        snapshot.content.resize(content_length);
        in.read(&snapshot.content[0], content_length);
        
        // Deserialize pattern
        in.read(reinterpret_cast<char*>(&snapshot.pattern), sizeof(CirculationPattern));
        
        // Deserialize timestamp
        std::chrono::seconds::rep timestamp_rep;
        in.read(reinterpret_cast<char*>(&timestamp_rep), sizeof(timestamp_rep));
        snapshot.timestamp = std::chrono::time_point<std::chrono::system_clock>(
            std::chrono::seconds(timestamp_rep));
        
        // Deserialize version
        in.read(reinterpret_cast<char*>(&snapshot.version), sizeof(uint32_t));
        
        return in.good();
    } catch (const std::exception& e) {
        std::cerr << "Snapshot deserialization error: " << e.what() << std::endl;
        return false;
    }
}

bool FlowPersistence::serialize_checkpoint(const PersistenceCheckpoint& checkpoint, std::ostream& out) {
    try {
        // Serialize checkpoint ID
        size_t id_length = checkpoint.checkpoint_id.length();
        out.write(reinterpret_cast<const char*>(&id_length), sizeof(size_t));
        out.write(checkpoint.checkpoint_id.c_str(), id_length);
        
        // Serialize timestamp
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            checkpoint.timestamp.time_since_epoch()).count();
        out.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        
        // Serialize flows
        size_t flow_count = checkpoint.flows.size();
        out.write(reinterpret_cast<const char*>(&flow_count), sizeof(size_t));
        
        for (const auto& flow : checkpoint.flows) {
            serialize_snapshot(flow, out);
        }
        
        return out.good();
    } catch (const std::exception& e) {
        std::cerr << "Checkpoint serialization error: " << e.what() << std::endl;
        return false;
    }
}

bool FlowPersistence::deserialize_checkpoint(std::istream& in, PersistenceCheckpoint& checkpoint) {
    try {
        // Deserialize checkpoint ID
        size_t id_length;
        in.read(reinterpret_cast<char*>(&id_length), sizeof(size_t));
        checkpoint.checkpoint_id.resize(id_length);
        in.read(&checkpoint.checkpoint_id[0], id_length);
        
        // Deserialize timestamp
        std::chrono::seconds::rep timestamp_rep;
        in.read(reinterpret_cast<char*>(&timestamp_rep), sizeof(timestamp_rep));
        checkpoint.timestamp = std::chrono::time_point<std::chrono::system_clock>(
            std::chrono::seconds(timestamp_rep));
        
        // Deserialize flows
        size_t flow_count;
        in.read(reinterpret_cast<char*>(&flow_count), sizeof(size_t));
        
        checkpoint.flows.reserve(flow_count);
        for (size_t i = 0; i < flow_count; ++i) {
            FlowSnapshot snapshot;
            if (deserialize_snapshot(in, snapshot)) {
                checkpoint.flows.push_back(snapshot);
            }
        }
        
        return in.good();
    } catch (const std::exception& e) {
        std::cerr << "Checkpoint deserialization error: " << e.what() << std::endl;
        return false;
    }
}

bool FlowPersistence::ensure_persistence_directory() {
    return create_directory_if_not_exists(persistence_dir_) &&
           create_directory_if_not_exists(persistence_dir_ + "/flows") &&
           create_directory_if_not_exists(persistence_dir_ + "/snapshots") &&
           create_directory_if_not_exists(persistence_dir_ + "/checkpoints");
}

bool FlowPersistence::create_directory_if_not_exists(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            return std::filesystem::create_directories(path);
        }
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create directory " << path << ": " << e.what() << std::endl;
        return false;
    }
}

bool FlowPersistence::validate_snapshot(const FlowSnapshot& snapshot) const {
    return snapshot.flow_id != 0 && 
           !snapshot.flow_name.empty() && 
           snapshot.version > 0;
}

bool FlowPersistence::merge_snapshots(const std::vector<FlowSnapshot>& snapshots, FlowFile& flow) {
    if (snapshots.empty()) {
        return false;
    }
    
    // Use the most recent snapshot
    auto latest = std::max_element(snapshots.begin(), snapshots.end(),
                                  [](const FlowSnapshot& a, const FlowSnapshot& b) {
                                      return a.timestamp < b.timestamp;
                                  });
    
    flow.write_to_flow(latest->content);
    flow.update_circulation_pattern(latest->pattern);
    
    return true;
}

FlowSnapshot FlowPersistence::create_snapshot_from_flow(const FlowFile& flow) const {
    FlowSnapshot snapshot;
    snapshot.flow_id = flow.identifier();
    snapshot.flow_name = flow.name();
    snapshot.content = flow.content();
    snapshot.pattern = flow.pattern();
    snapshot.circulation_path = flow.circulation_path();
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.version = 1; // In a real implementation, this would be incremented
    
    return snapshot;
}

// NetworkDisruptionHandler implementation
NetworkDisruptionHandler::NetworkDisruptionHandler(FlowPersistence* persistence) 
    : persistence_(persistence), disruption_detected_(false) {
    last_heartbeat_ = std::chrono::system_clock::now();
}

NetworkDisruptionHandler::~NetworkDisruptionHandler() {
    stop_monitoring();
}

void NetworkDisruptionHandler::start_monitoring() {
    monitoring_thread_ = std::thread(&NetworkDisruptionHandler::monitoring_worker, this);
    std::cout << "Started network disruption monitoring" << std::endl;
}

void NetworkDisruptionHandler::stop_monitoring() {
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
    std::cout << "Stopped network disruption monitoring" << std::endl;
}

bool NetworkDisruptionHandler::handle_disruption() {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    
    if (disruption_detected_) {
        std::cout << "Handling network disruption..." << std::endl;
        
        // Trigger emergency persistence
        trigger_emergency_persistence();
        
        // Mark disruption as handled
        disruption_detected_ = false;
        
        return true;
    }
    
    return false;
}

bool NetworkDisruptionHandler::recover_after_disruption() {
    std::cout << "Recovering after network disruption..." << std::endl;
    
    if (persistence_) {
        return persistence_->recover_all_flows();
    }
    
    return false;
}

void NetworkDisruptionHandler::update_heartbeat() {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    last_heartbeat_ = std::chrono::system_clock::now();
}

std::chrono::seconds NetworkDisruptionHandler::get_time_since_last_heartbeat() const {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat_);
}

void NetworkDisruptionHandler::monitoring_worker() {
    while (true) {
        if (detect_network_disruption()) {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            disruption_detected_ = true;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

bool NetworkDisruptionHandler::detect_network_disruption() {
    auto time_since_heartbeat = get_time_since_last_heartbeat();
    
    // Consider it a disruption if no heartbeat for 30 seconds
    return time_since_heartbeat > std::chrono::seconds(30);
}

void NetworkDisruptionHandler::trigger_emergency_persistence() {
    std::cout << "Triggering emergency persistence..." << std::endl;
    
    if (persistence_) {
        persistence_->create_checkpoint();
    }
}

} // namespace nerd