#pragma once

#include "core/flow_file.h"
#include "network/flow.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

namespace nerd {

// Network topology information
struct NetworkTopology {
    std::vector<std::string> interfaces;
    std::vector<std::string> discovered_nodes;
    std::map<std::string, std::vector<std::string>> routing_table;
    
    NetworkTopology() = default;
};

// FlowManager class - manages active files and network topology
class FlowManager {
private:
    std::map<std::string, std::unique_ptr<FlowFile>> active_files_;
    NetworkTopology topology_;
    std::unique_ptr<NetworkFlow> network_flow_;
    
    // Flow discovery and maintenance
    std::thread discovery_thread_;
    std::atomic<bool> running_;
    
    // Flow ID generation
    std::atomic<FlowID> next_flow_id_;
    
public:
    FlowManager();
    ~FlowManager();
    
    // File management
    FlowFile* open_flow(const std::string& flow_name);
    void close_flow(const std::string& flow_name);
    void create_circulation_pattern(const std::string& name);
    void sustain_all_flows();
    
    // Network management
    bool initialize_network(const std::string& interface);
    void handle_topology_change();
    void discover_network_topology();
    
    // Flow discovery
    std::vector<std::string> discover_existing_flows();
    bool connect_to_flow(const std::string& flow_name);
    
    // Accessors
    const NetworkTopology& topology() const { return topology_; }
    std::vector<std::string> get_active_flow_names() const;
    FlowFile* get_flow(const std::string& name);
    
    // Flow ID management
    FlowID generate_flow_id() { return next_flow_id_++; }
    
    // Network flow access
    NetworkFlow* network_flow() { return network_flow_.get(); }
    
private:
    void discovery_worker();
    void maintain_flow_circulation();
    void handle_flow_discovery_response(const RawPacket& packet);
    void broadcast_flow_existence(const std::string& flow_name);
    bool validate_flow_name(const std::string& name) const;
};

} // namespace nerd