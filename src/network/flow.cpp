#include "network/flow.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <algorithm>

namespace nerd {

NetworkFlow::NetworkFlow() : running_(false), raw_socket_(-1) {}

NetworkFlow::~NetworkFlow() {
    stop_circulation();
    close_interface();
}

void NetworkFlow::inject_packet(const RawPacket& packet) {
    std::lock_guard<std::mutex> lock(streams_mutex_);
    
    // Find or create stream for this flow
    FlowID flow_id = packet.header().flow_id;
    PacketStream* stream = get_stream(flow_id);
    
    if (!stream) {
        add_stream(flow_id);
        stream = get_stream(flow_id);
    }
    
    if (stream) {
        stream->add_packet(packet);
    }
    
    // Send packet to network
    send_raw_packet(packet);
}

void NetworkFlow::modify_flow_pattern(FlowID id, const CirculationPattern& new_pattern) {
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    flow_patterns_[id] = new_pattern;
}

void NetworkFlow::sustain_circulation() {
    std::lock_guard<std::mutex> lock(streams_mutex_);
    
    for (auto& stream : active_streams_) {
        if (stream) {
            stream->maintain_circulation();
            stream->cleanup_old_packets();
        }
    }
}

void NetworkFlow::add_stream(FlowID flow_id) {
    active_streams_.push_back(std::make_unique<PacketStream>(flow_id));
}

void NetworkFlow::remove_stream(FlowID flow_id) {
    active_streams_.erase(
        std::remove_if(active_streams_.begin(), active_streams_.end(),
                      [flow_id](const std::unique_ptr<PacketStream>& stream) {
                          return stream && stream->flow_id() == flow_id;
                      }),
        active_streams_.end()
    );
}

PacketStream* NetworkFlow::get_stream(FlowID flow_id) {
    for (auto& stream : active_streams_) {
        if (stream && stream->flow_id() == flow_id) {
            return stream.get();
        }
    }
    return nullptr;
}

void NetworkFlow::add_circulation_pattern(const CirculationPattern& pattern) {
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    flow_patterns_[pattern.id] = pattern;
}

void NetworkFlow::remove_circulation_pattern(FlowID id) {
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    flow_patterns_.erase(id);
}

CirculationPattern* NetworkFlow::get_pattern(FlowID id) {
    std::lock_guard<std::mutex> lock(patterns_mutex_);
    auto it = flow_patterns_.find(id);
    return (it != flow_patterns_.end()) ? &it->second : nullptr;
}

bool NetworkFlow::initialize_interface(const std::string& interface) {
    interface_name_ = interface;
    
    // Create raw socket
    raw_socket_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (raw_socket_ < 0) {
        std::cerr << "Failed to create raw socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Get interface index
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    
    if (ioctl(raw_socket_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "Failed to get interface index: " << strerror(errno) << std::endl;
        close(raw_socket_);
        raw_socket_ = -1;
        return false;
    }
    
    // Bind socket to interface
    struct sockaddr_ll addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = ifr.ifr_ifindex;
    
    if (bind(raw_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
        close(raw_socket_);
        raw_socket_ = -1;
        return false;
    }
    
    std::cout << "Initialized interface: " << interface << std::endl;
    return true;
}

void NetworkFlow::close_interface() {
    if (raw_socket_ >= 0) {
        close(raw_socket_);
        raw_socket_ = -1;
    }
}

void NetworkFlow::start_circulation() {
    if (!running_) {
        running_ = true;
        circulation_thread_ = std::thread(&NetworkFlow::circulation_worker, this);
        std::cout << "Started circulation worker thread" << std::endl;
    }
}

void NetworkFlow::stop_circulation() {
    if (running_) {
        running_ = false;
        circulation_cv_.notify_all();
        
        if (circulation_thread_.joinable()) {
            circulation_thread_.join();
        }
        
        std::cout << "Stopped circulation worker thread" << std::endl;
    }
}

void NetworkFlow::discover_flows() {
    // Send discovery packets to find existing flows
    RawPacket discovery_packet(0, FLOW_DISCOVERY, {});
    send_raw_packet(discovery_packet);
}

std::vector<FlowID> NetworkFlow::get_active_flows() const {
    std::vector<FlowID> flows;
    std::lock_guard<std::mutex> lock(streams_mutex_);
    
    for (const auto& stream : active_streams_) {
        if (stream) {
            flows.push_back(stream->flow_id());
        }
    }
    
    return flows;
}

void NetworkFlow::circulation_worker() {
    while (running_) {
        // Sustain all active flows
        sustain_circulation();
        
        // Maintain flow patterns
        {
            std::lock_guard<std::mutex> lock(patterns_mutex_);
            for (const auto& pair : flow_patterns_) {
                if (pair.second.auto_sustain) {
                    maintain_flow_pattern(pair.first);
                }
            }
        }
        
        // Sleep for a short interval
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool NetworkFlow::send_raw_packet(const RawPacket& packet) {
    if (raw_socket_ < 0) {
        return false;
    }
    
    std::vector<uint8_t> serialized = packet.serialize();
    
    // Create Ethernet frame
    struct ethernet_header {
        uint8_t dest[6];
        uint8_t source[6];
        uint16_t type;
    } __attribute__((packed));
    
    // For simplicity, use broadcast address
    struct ethernet_header eth;
    std::memset(eth.dest, 0xFF, 6);  // Broadcast
    std::memset(eth.source, 0x00, 6); // Placeholder
    eth.type = htons(0x1234);  // Custom protocol type
    
    // Combine Ethernet header with packet data
    std::vector<uint8_t> frame;
    frame.reserve(sizeof(ethernet_header) + serialized.size());
    
    const uint8_t* eth_ptr = reinterpret_cast<const uint8_t*>(&eth);
    frame.insert(frame.end(), eth_ptr, eth_ptr + sizeof(ethernet_header));
    frame.insert(frame.end(), serialized.begin(), serialized.end());
    
    // Send frame
    ssize_t sent = send(raw_socket_, frame.data(), frame.size(), 0);
    return sent == static_cast<ssize_t>(frame.size());
}

void NetworkFlow::handle_incoming_packet(const RawPacket& packet) {
    // Handle different packet types
    switch (static_cast<PacketType>(packet.header().packet_type)) {
        case FLOW_DATA:
            inject_packet(packet);
            break;
            
        case FLOW_DISCOVERY:
            // Respond with our active flows
            discover_flows();
            break;
            
        case FLOW_HEARTBEAT:
            // Update flow timestamp
            inject_packet(packet);
            break;
            
        case FLOW_EDIT:
            // Handle edit commands
            inject_packet(packet);
            break;
            
        default:
            break;
    }
}

void NetworkFlow::maintain_flow_pattern(FlowID flow_id) {
    CirculationPattern* pattern = get_pattern(flow_id);
    if (!pattern) {
        return;
    }
    
    PacketStream* stream = get_stream(flow_id);
    if (!stream) {
        return;
    }
    
    // Send heartbeat packets to maintain circulation
    std::vector<uint8_t> heartbeat_data = {'H', 'B'};
    RawPacket heartbeat(flow_id, FLOW_HEARTBEAT, heartbeat_data);
    heartbeat.set_sequence(stream->next_sequence());
    
    send_raw_packet(heartbeat);
    stream->add_packet(heartbeat);
}

} // namespace nerd