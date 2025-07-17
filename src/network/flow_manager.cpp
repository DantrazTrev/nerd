#include "network/flow_manager.h"
#include <iostream>
#include <algorithm>
#include <chrono>

namespace nerd {

FlowManager::FlowManager() : running_(false), next_flow_id_(1) {
    network_flow_ = std::make_unique<NetworkFlow>();
}

FlowManager::~FlowManager() {
    if (running_) {
        running_ = false;
        if (discovery_thread_.joinable()) {
            discovery_thread_.join();
        }
    }
}

FlowFile* FlowManager::open_flow(const std::string& flow_name) {
    if (!validate_flow_name(flow_name)) {
        std::cerr << "Invalid flow name: " << flow_name << std::endl;
        return nullptr;
    }
    
    // Check if flow already exists
    auto it = active_files_.find(flow_name);
    if (it != active_files_.end()) {
        return it->second.get();
    }
    
    // Try to discover existing flow
    if (connect_to_flow(flow_name)) {
        return get_flow(flow_name);
    }
    
    // Create new flow
    FlowID flow_id = generate_flow_id();
    auto flow_file = std::make_unique<FlowFile>(flow_id, flow_name);
    
    // Set up circulation pattern
    CirculationPattern pattern;
    pattern.id = flow_id;
    pattern.name = flow_name;
    pattern.circulation_rate = 10; // 10 packets per second
    pattern.auto_sustain = true;
    
    flow_file->update_circulation_pattern(pattern);
    
    // Add to network flow
    if (network_flow_) {
        network_flow_->add_circulation_pattern(pattern);
    }
    
    // Store the flow
    FlowFile* result = flow_file.get();
    active_files_[flow_name] = std::move(flow_file);
    
    std::cout << "Created new flow: " << flow_name << " (ID: " << flow_id << ")" << std::endl;
    return result;
}

void FlowManager::close_flow(const std::string& flow_name) {
    auto it = active_files_.find(flow_name);
    if (it != active_files_.end()) {
        FlowID flow_id = it->second->identifier();
        
        // Remove from network flow
        if (network_flow_) {
            network_flow_->remove_circulation_pattern(flow_id);
            network_flow_->remove_stream(flow_id);
        }
        
        // Remove from active files
        active_files_.erase(it);
        
        std::cout << "Closed flow: " << flow_name << std::endl;
    }
}

void FlowManager::create_circulation_pattern(const std::string& name) {
    // This would create a new circulation pattern without a file
    CirculationPattern pattern;
    pattern.id = generate_flow_id();
    pattern.name = name;
    pattern.circulation_rate = 5;
    pattern.auto_sustain = true;
    
    if (network_flow_) {
        network_flow_->add_circulation_pattern(pattern);
    }
    
    std::cout << "Created circulation pattern: " << name << std::endl;
}

void FlowManager::sustain_all_flows() {
    for (auto& pair : active_files_) {
        if (pair.second) {
            pair.second->maintain_flow();
        }
    }
    
    if (network_flow_) {
        network_flow_->sustain_circulation();
    }
}

bool FlowManager::initialize_network(const std::string& interface) {
    if (!network_flow_) {
        return false;
    }
    
    bool success = network_flow_->initialize_interface(interface);
    if (success) {
        // Add interface to topology
        topology_.interfaces.push_back(interface);
        
        // Start network flow
        network_flow_->start_circulation();
        
        // Start discovery thread
        running_ = true;
        discovery_thread_ = std::thread(&FlowManager::discovery_worker, this);
        
        std::cout << "Initialized network on interface: " << interface << std::endl;
    }
    
    return success;
}

void FlowManager::handle_topology_change() {
    // Discover new nodes and update routing
    discover_network_topology();
    
    // Adapt circulation patterns to new topology
    for (auto& pair : active_files_) {
        if (pair.second) {
            // Update circulation paths based on new topology
            CirculationPattern pattern = pair.second->pattern();
            // In a real implementation, this would update the pattern
            // based on available network paths
            pair.second->update_circulation_pattern(pattern);
        }
    }
}

void FlowManager::discover_network_topology() {
    // In a real implementation, this would:
    // 1. Send discovery packets to find other nodes
    // 2. Build routing table based on responses
    // 3. Update topology information
    
    std::cout << "Discovering network topology..." << std::endl;
    
    // For now, just simulate discovery
    topology_.discovered_nodes.clear();
    topology_.routing_table.clear();
    
    // Simulate finding some nodes
    topology_.discovered_nodes = {"192.168.1.100", "192.168.1.101", "192.168.1.102"};
    
    // Build simple routing table
    for (const auto& node : topology_.discovered_nodes) {
        topology_.routing_table[node] = topology_.discovered_nodes;
    }
}

std::vector<std::string> FlowManager::discover_existing_flows() {
    std::vector<std::string> discovered_flows;
    
    if (network_flow_) {
        network_flow_->discover_flows();
        
        // In a real implementation, this would wait for responses
        // and collect flow names from discovery packets
        
        // For now, return empty list
    }
    
    return discovered_flows;
}

bool FlowManager::connect_to_flow(const std::string& flow_name) {
    // Try to discover and connect to existing flow
    std::cout << "Attempting to connect to existing flow: " << flow_name << std::endl;
    
    // In a real implementation, this would:
    // 1. Send discovery packets for this specific flow
    // 2. Wait for responses
    // 3. Create FlowFile from discovered flow data
    
    // For now, assume no existing flow
    return false;
}

std::vector<std::string> FlowManager::get_active_flow_names() const {
    std::vector<std::string> names;
    names.reserve(active_files_.size());
    
    for (const auto& pair : active_files_) {
        names.push_back(pair.first);
    }
    
    return names;
}

FlowFile* FlowManager::get_flow(const std::string& name) {
    auto it = active_files_.find(name);
    return (it != active_files_.end()) ? it->second.get() : nullptr;
}

void FlowManager::discovery_worker() {
    while (running_) {
        // Periodically discover network topology
        discover_network_topology();
        
        // Maintain flow circulation
        maintain_flow_circulation();
        
        // Sleep for discovery interval
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void FlowManager::maintain_flow_circulation() {
    // Sustain all active flows
    sustain_all_flows();
    
    // Broadcast existence of our flows
    for (const auto& pair : active_files_) {
        if (pair.second) {
            broadcast_flow_existence(pair.first);
        }
    }
}

void FlowManager::handle_flow_discovery_response(const RawPacket& packet) {
    // Handle discovery response packets
    // In a real implementation, this would parse the response
    // and update flow information accordingly
    
    std::cout << "Received flow discovery response" << std::endl;
}

void FlowManager::broadcast_flow_existence(const std::string& flow_name) {
    // Broadcast that this flow exists
    // In a real implementation, this would send broadcast packets
    // to announce the flow's existence
    
    std::cout << "Broadcasting existence of flow: " << flow_name << std::endl;
}

bool FlowManager::validate_flow_name(const std::string& name) const {
    // Basic validation - no empty names, no special characters
    if (name.empty()) {
        return false;
    }
    
    // Check for invalid characters
    for (char c : name) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || 
            c == '"' || c == '<' || c == '>' || c == '|') {
            return false;
        }
    }
    
    return true;
}

} // namespace nerd