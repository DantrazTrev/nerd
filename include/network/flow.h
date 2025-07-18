#pragma once

#include "network/packet.h"
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace nerd {

// Circulation pattern - defines how data flows through the network
struct CirculationPattern {
    FlowID id;
    std::string name;
    std::vector<std::string> nodes;  // Network nodes in the circulation path
    uint32_t circulation_rate;       // Packets per second to maintain flow
    uint32_t max_packet_age;         // Maximum age of packets in microseconds
    bool auto_sustain;               // Whether to automatically sustain the flow
    
    CirculationPattern() : id(0), circulation_rate(10), max_packet_age(30000000), auto_sustain(true) {}
};

// Network node information
struct NetworkNode {
    std::string address;
    uint16_t port;
    std::string interface;
    bool is_local;
    
    NetworkNode() : port(0), is_local(false) {}
};

// NetworkFlow class - manages active streams and circulation patterns
class NetworkFlow {
private:
    std::vector<std::unique_ptr<PacketStream>> active_streams_;
    std::map<FlowID, CirculationPattern> flow_patterns_;
    std::map<FlowID, std::vector<NetworkNode>> circulation_paths_;
    
    // Threading for flow maintenance
    std::thread circulation_thread_;
    std::atomic<bool> running_;
    mutable std::mutex streams_mutex_;
    mutable std::mutex patterns_mutex_;
    std::condition_variable circulation_cv_;
    
    // Raw socket for packet injection
    int raw_socket_;
    std::string interface_name_;
    
public:
    NetworkFlow();
    ~NetworkFlow();
    
    // Flow management
    void inject_packet(const RawPacket& packet);
    void modify_flow_pattern(FlowID id, const CirculationPattern& new_pattern);
    void sustain_circulation();
    
    // Stream management
    void add_stream(FlowID flow_id);
    void remove_stream(FlowID flow_id);
    PacketStream* get_stream(FlowID flow_id);
    
    // Pattern management
    void add_circulation_pattern(const CirculationPattern& pattern);
    void remove_circulation_pattern(FlowID id);
    CirculationPattern* get_pattern(FlowID id);
    
    // Network interface
    bool initialize_interface(const std::string& interface);
    void close_interface();
    
    // Circulation control
    void start_circulation();
    void stop_circulation();
    
    // Discovery
    void discover_flows();
    std::vector<FlowID> get_active_flows() const;
    
private:
    void circulation_worker();
    bool send_raw_packet(const RawPacket& packet);
    void handle_incoming_packet(const RawPacket& packet);
    void maintain_flow_pattern(FlowID flow_id);
};

} // namespace nerd