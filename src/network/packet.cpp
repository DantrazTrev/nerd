#include "network/packet.h"
#include <cstring>
#include <chrono>
#include <algorithm>

namespace nerd {

// Magic number for flow packet identification
static const uint32_t FLOW_MAGIC = 0x4E455244; // "NERD" in ASCII

RawPacket::RawPacket() {
    header_.magic = FLOW_MAGIC;
    header_.flow_id = 0;
    header_.sequence = 0;
    header_.packet_type = FLOW_DATA;
    header_.data_length = 0;
    header_.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

RawPacket::RawPacket(FlowID flow_id, PacketType type, const std::vector<uint8_t>& payload) 
    : data_(payload) {
    header_.magic = FLOW_MAGIC;
    header_.flow_id = flow_id;
    header_.sequence = 0;
    header_.packet_type = static_cast<uint32_t>(type);
    header_.data_length = payload.size();
    header_.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void RawPacket::set_header(const FlowPacketHeader& header) {
    header_ = header;
}

void RawPacket::set_payload(const std::vector<uint8_t>& payload) {
    data_ = payload;
    header_.data_length = payload.size();
}

void RawPacket::set_flow_id(FlowID flow_id) {
    header_.flow_id = flow_id;
}

void RawPacket::set_packet_type(PacketType type) {
    header_.packet_type = static_cast<uint32_t>(type);
}

void RawPacket::set_sequence(uint32_t seq) {
    header_.sequence = seq;
}

std::vector<uint8_t> RawPacket::serialize() const {
    std::vector<uint8_t> serialized;
    serialized.reserve(sizeof(FlowPacketHeader) + data_.size());
    
    // Serialize header
    const uint8_t* header_ptr = reinterpret_cast<const uint8_t*>(&header_);
    serialized.insert(serialized.end(), header_ptr, header_ptr + sizeof(FlowPacketHeader));
    
    // Serialize payload
    serialized.insert(serialized.end(), data_.begin(), data_.end());
    
    return serialized;
}

bool RawPacket::deserialize(const std::vector<uint8_t>& raw_data) {
    if (raw_data.size() < sizeof(FlowPacketHeader)) {
        return false;
    }
    
    // Deserialize header
    std::memcpy(&header_, raw_data.data(), sizeof(FlowPacketHeader));
    
    // Validate magic number
    if (header_.magic != FLOW_MAGIC) {
        return false;
    }
    
    // Deserialize payload
    if (raw_data.size() >= sizeof(FlowPacketHeader) + header_.data_length) {
        data_.assign(raw_data.begin() + sizeof(FlowPacketHeader), 
                    raw_data.begin() + sizeof(FlowPacketHeader) + header_.data_length);
    }
    
    return true;
}

bool RawPacket::is_valid() const {
    return header_.magic == FLOW_MAGIC && 
           header_.data_length == data_.size();
}

bool RawPacket::is_flow_packet() const {
    return header_.magic == FLOW_MAGIC;
}

// PacketStream implementation
PacketStream::PacketStream(FlowID flow_id) : flow_id_(flow_id), next_sequence_(0) {}

void PacketStream::add_packet(const RawPacket& packet) {
    if (packet.header().flow_id == flow_id_) {
        packets_.push_back(packet);
        
        // Maintain packet order by sequence number
        std::sort(packets_.begin(), packets_.end(), 
                 [](const RawPacket& a, const RawPacket& b) {
                     return a.header().sequence < b.header().sequence;
                 });
    }
}

void PacketStream::remove_packet(uint32_t sequence) {
    packets_.erase(
        std::remove_if(packets_.begin(), packets_.end(),
                      [sequence](const RawPacket& packet) {
                          return packet.header().sequence == sequence;
                      }),
        packets_.end()
    );
}

void PacketStream::maintain_circulation() {
    // Update timestamps for all packets to keep them "alive"
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
    
    for (auto& packet : packets_) {
        // Create a new packet with updated timestamp
        RawPacket new_packet;
        new_packet.set_header(packet.header());
        new_packet.set_payload(packet.data());
        new_packet.set_sequence(packet.header().sequence);
        
        // Update timestamp in header
        FlowPacketHeader header = new_packet.header();
        header.timestamp = timestamp;
        new_packet.set_header(header);
        
        packet = new_packet;
    }
}

void PacketStream::cleanup_old_packets() {
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t current_time = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
    
    // Remove packets older than 30 seconds
    const uint64_t MAX_AGE_MICROSECONDS = 30000000;
    
    packets_.erase(
        std::remove_if(packets_.begin(), packets_.end(),
                      [current_time](const RawPacket& packet) {
                          return (current_time - packet.header().timestamp) > MAX_AGE_MICROSECONDS;
                      }),
        packets_.end()
    );
}

} // namespace nerd