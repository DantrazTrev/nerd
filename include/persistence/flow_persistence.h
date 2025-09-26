#pragma once

#include "core/flow_file.h"
#include "network/flow.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <chrono>
#include <mutex>

namespace nerd {

// Persistence strategy types
enum class PersistenceStrategy {
    MEMORY_ONLY,        // Keep only in memory (fastest, least reliable)
    LOCAL_DISK,         // Persist to local disk
    DISTRIBUTED,        // Distribute across multiple nodes
    HYBRID              // Combination of strategies
};

// Flow snapshot for persistence
struct FlowSnapshot {
    FlowID flow_id;
    std::string flow_name;
    std::string content;
    CirculationPattern pattern;
    std::vector<NetworkNode> circulation_path;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    uint32_t version;
    std::map<std::string, std::string> metadata;
    
    FlowSnapshot() : flow_id(0), version(1) {}
};

// Persistence checkpoint
struct PersistenceCheckpoint {
    std::string checkpoint_id;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::vector<FlowSnapshot> flows;
    std::map<std::string, std::string> system_state;
    
    PersistenceCheckpoint() {}
};

// Flow persistence manager - handles flow persistence and recovery
class FlowPersistence {
private:
    std::string persistence_dir_;
    PersistenceStrategy strategy_;
    std::map<FlowID, FlowSnapshot> snapshots_;
    std::vector<PersistenceCheckpoint> checkpoints_;
    mutable std::mutex persistence_mutex_;
    
    // Configuration
    std::chrono::seconds checkpoint_interval_;
    size_t max_checkpoints_;
    bool auto_persistence_;
    
public:
    FlowPersistence(const std::string& persistence_dir = "/tmp/nerd_persistence");
    ~FlowPersistence();
    
    // Configuration
    void set_strategy(PersistenceStrategy strategy) { strategy_ = strategy; }
    void set_checkpoint_interval(std::chrono::seconds interval) { checkpoint_interval_ = interval; }
    void set_max_checkpoints(size_t max) { max_checkpoints_ = max; }
    void set_auto_persistence(bool enabled) { auto_persistence_ = enabled; }
    
    // Flow persistence
    bool save_flow(const FlowFile& flow);
    bool load_flow(FlowID flow_id, FlowFile& flow);
    bool delete_flow(FlowID flow_id);
    std::vector<FlowID> list_persisted_flows() const;
    
    // Snapshot management
    bool create_snapshot(const FlowFile& flow);
    bool restore_from_snapshot(FlowID flow_id, FlowFile& flow);
    std::vector<FlowSnapshot> get_flow_history(FlowID flow_id) const;
    
    // Checkpoint management
    bool create_checkpoint();
    bool restore_from_checkpoint(const std::string& checkpoint_id);
    std::vector<std::string> list_checkpoints() const;
    bool delete_checkpoint(const std::string& checkpoint_id);
    
    // Recovery operations
    bool recover_all_flows();
    bool recover_flow(FlowID flow_id);
    std::vector<FlowID> get_recoverable_flows() const;
    
    // Persistence status
    bool is_flow_persisted(FlowID flow_id) const;
    std::chrono::time_point<std::chrono::system_clock> get_last_save_time(FlowID flow_id) const;
    size_t get_persistence_size() const;
    
    // Maintenance
    void cleanup_old_snapshots();
    void compact_persistence();
    void verify_persistence_integrity();
    
private:
    // File operations
    std::string get_flow_file_path(FlowID flow_id) const;
    std::string get_snapshot_file_path(FlowID flow_id, uint32_t version) const;
    std::string get_checkpoint_file_path(const std::string& checkpoint_id) const;
    
    // Serialization
    bool serialize_flow(const FlowFile& flow, std::ostream& out);
    bool deserialize_flow(std::istream& in, FlowFile& flow);
    bool serialize_snapshot(const FlowSnapshot& snapshot, std::ostream& out);
    bool deserialize_snapshot(std::istream& in, FlowSnapshot& snapshot);
    bool serialize_checkpoint(const PersistenceCheckpoint& checkpoint, std::ostream& out);
    bool deserialize_checkpoint(std::istream& in, PersistenceCheckpoint& checkpoint);
    
    // Directory operations
    bool ensure_persistence_directory();
    bool create_directory_if_not_exists(const std::string& path);
    
    // Recovery helpers
    bool validate_snapshot(const FlowSnapshot& snapshot) const;
    bool merge_snapshots(const std::vector<FlowSnapshot>& snapshots, FlowFile& flow);
    FlowSnapshot create_snapshot_from_flow(const FlowFile& flow) const;
};

// Network disruption handler
class NetworkDisruptionHandler {
private:
    FlowPersistence* persistence_;
    std::atomic<bool> disruption_detected_;
    std::chrono::time_point<std::chrono::system_clock> last_heartbeat_;
    std::thread monitoring_thread_;
    mutable std::mutex handler_mutex_;
    
public:
    NetworkDisruptionHandler(FlowPersistence* persistence);
    ~NetworkDisruptionHandler();
    
    // Disruption detection
    void start_monitoring();
    void stop_monitoring();
    bool is_disruption_detected() const { return disruption_detected_; }
    
    // Recovery operations
    bool handle_disruption();
    bool recover_after_disruption();
    
    // Heartbeat management
    void update_heartbeat();
    std::chrono::seconds get_time_since_last_heartbeat() const;
    
private:
    void monitoring_worker();
    bool detect_network_disruption();
    void trigger_emergency_persistence();
};

} // namespace nerd