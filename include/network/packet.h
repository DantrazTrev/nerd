#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

namespace nerd {

// Flow identifier - unique identifier for a circulation pattern
using FlowID = uint64_t;

// Custom packet header for flow identification
struct FlowPacketHeader {
    uint32_t magic;           // Magic number to identify flow packets
    FlowID flow_id;           // Unique flow identifier
    uint32_t sequence;        // Sequence number within the flow
    uint32_t packet_type;     // Type of packet (data, control, heartbeat)
    uint32_t data_length;     // Length of payload data
    uint64_t timestamp;       // Timestamp for flow timing
} __attribute__((packed));

// Packet types
enum PacketType {
    FLOW_DATA = 0x01,         // Data packet carrying file content
    FLOW_CONTROL = 0x02,      // Control packet for flow management
    FLOW_HEARTBEAT = 0x03,    // Heartbeat to maintain circulation
    FLOW_EDIT = 0x04,         // Edit command packet
    FLOW_DISCOVERY = 0x05     // Flow discovery packet
};

// Raw packet representation
class RawPacket {
private:
    std::vector<uint8_t> data_;
    FlowPacketHeader header_;

public:
    RawPacket();
    RawPacket(FlowID flow_id, PacketType type, const std::vector<uint8_t>& payload);
    
    // Packet construction
    void set_header(const FlowPacketHeader& header);
    void set_payload(const std::vector<uint8_t>& payload);
    void set_flow_id(FlowID flow_id);
    void set_packet_type(PacketType type);
    void set_sequence(uint32_t seq);
    
    // Packet access
    const FlowPacketHeader& header() const { return header_; }
    const std::vector<uint8_t>& data() const { return data_; }
    std::vector<uint8_t>& data() { return data_; }
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& raw_data);
    
    // Validation
    bool is_valid() const;
    bool is_flow_packet() const;
};

// Packet stream for managing packet flow
class PacketStream {
private:
    FlowID flow_id_;
    std::vector<RawPacket> packets_;
    uint32_t next_sequence_;
    
public:
    explicit PacketStream(FlowID flow_id);
    
    void add_packet(const RawPacket& packet);
    void remove_packet(uint32_t sequence);
    const std::vector<RawPacket>& packets() const { return packets_; }
    FlowID flow_id() const { return flow_id_; }
    uint32_t next_sequence() { return next_sequence_++; }
    
    // Flow management
    void maintain_circulation();
    void cleanup_old_packets();
};

} // namespace nerd