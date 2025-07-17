#include "core/flow_file.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace nerd {

FlowFile::FlowFile(FlowID id, const std::string& name) 
    : identifier_(id), name_(name), is_modified_(false) {
    pattern_.id = id;
    pattern_.name = name;
}

FlowFile::~FlowFile() = default;

void FlowFile::maintain_flow() {
    // Encode current content into network packets
    encode_content_in_packets();
    
    // Update circulation pattern if needed
    if (is_modified_) {
        broadcast_existence();
        is_modified_ = false;
    }
}

void FlowFile::modify_pattern(const EditCommand& cmd) {
    switch (cmd.type) {
        case EditCommand::APPEND:
            append_content(cmd.data);
            break;
            
        case EditCommand::DELETE:
            delete_content(cmd.start_line, cmd.end_line);
            break;
            
        case EditCommand::SUBSTITUTE:
            substitute_content(cmd.pattern, cmd.replacement);
            break;
            
        case EditCommand::INSERT:
            insert_content(cmd.start_line, cmd.data);
            break;
    }
}

std::string FlowFile::read_from_flow() {
    // In a real implementation, this would read from the circulating packets
    // For now, return the current content
    return content_;
}

void FlowFile::write_to_flow(const std::string& data) {
    content_ = data;
    is_modified_ = true;
    notify_content_change();
}

void FlowFile::append_content(const std::string& line) {
    if (!content_.empty() && content_.back() != '\n') {
        content_ += '\n';
    }
    content_ += line;
    is_modified_ = true;
    notify_content_change();
}

void FlowFile::delete_content(int start_line, int end_line) {
    std::vector<std::string> lines = split_into_lines(content_);
    
    if (start_line >= 0 && start_line < static_cast<int>(lines.size()) &&
        end_line >= start_line && end_line < static_cast<int>(lines.size())) {
        
        lines.erase(lines.begin() + start_line, lines.begin() + end_line + 1);
        content_ = join_lines(lines);
        is_modified_ = true;
        notify_content_change();
    }
}

void FlowFile::substitute_content(const std::string& pattern, const std::string& replacement) {
    size_t pos = 0;
    while ((pos = content_.find(pattern, pos)) != std::string::npos) {
        content_.replace(pos, pattern.length(), replacement);
        pos += replacement.length();
    }
    is_modified_ = true;
    notify_content_change();
}

void FlowFile::insert_content(int line, const std::string& content) {
    std::vector<std::string> lines = split_into_lines(content_);
    
    if (line >= 0 && line <= static_cast<int>(lines.size())) {
        std::vector<std::string> insert_lines = split_into_lines(content);
        lines.insert(lines.begin() + line, insert_lines.begin(), insert_lines.end());
        this->content_ = join_lines(lines);
        is_modified_ = true;
        notify_content_change();
    }
}

void FlowFile::update_circulation_pattern(const CirculationPattern& pattern) {
    pattern_ = pattern;
    is_modified_ = true;
}

void FlowFile::add_circulation_node(const NetworkNode& node) {
    circulation_path_.push_back(node);
    is_modified_ = true;
}

void FlowFile::remove_circulation_node(const std::string& address) {
    circulation_path_.erase(
        std::remove_if(circulation_path_.begin(), circulation_path_.end(),
                      [&address](const NetworkNode& node) {
                          return node.address == address;
                      }),
        circulation_path_.end()
    );
    is_modified_ = true;
}

std::vector<uint8_t> FlowFile::serialize_content() const {
    std::vector<uint8_t> serialized;
    serialized.reserve(content_.size());
    
    for (char c : content_) {
        serialized.push_back(static_cast<uint8_t>(c));
    }
    
    return serialized;
}

bool FlowFile::deserialize_content(const std::vector<uint8_t>& data) {
    content_.clear();
    content_.reserve(data.size());
    
    for (uint8_t byte : data) {
        content_.push_back(static_cast<char>(byte));
    }
    
    is_modified_ = true;
    notify_content_change();
    return true;
}

bool FlowFile::discover_existing_flow() {
    // In a real implementation, this would send discovery packets
    // and wait for responses from existing flows
    std::cout << "Discovering existing flow: " << name_ << std::endl;
    return false; // Assume no existing flow for now
}

void FlowFile::broadcast_existence() {
    // In a real implementation, this would broadcast the flow's existence
    // to other nodes in the network
    std::cout << "Broadcasting flow existence: " << name_ << " (ID: " << identifier_ << ")" << std::endl;
}

void FlowFile::notify_content_change() {
    if (content_change_callback_) {
        content_change_callback_(content_);
    }
}

void FlowFile::encode_content_in_packets() {
    // In a real implementation, this would encode the content into
    // a series of packets that circulate through the network
    std::vector<uint8_t> content_data = serialize_content();
    
    // Split content into packet-sized chunks
    const size_t MAX_PACKET_SIZE = 1400; // Leave room for headers
    size_t offset = 0;
    uint32_t sequence = 0;
    
    while (offset < content_data.size()) {
        size_t chunk_size = std::min(MAX_PACKET_SIZE, content_data.size() - offset);
        std::vector<uint8_t> chunk(content_data.begin() + offset, 
                                  content_data.begin() + offset + chunk_size);
        
        // Create data packet
        RawPacket packet(identifier_, FLOW_DATA, chunk);
        packet.set_sequence(sequence++);
        
        // In a real implementation, this packet would be injected into the network
        // For now, just log it
        std::cout << "Encoded packet " << sequence - 1 << " for flow " << identifier_ 
                  << " with " << chunk_size << " bytes" << std::endl;
        
        offset += chunk_size;
    }
}

void FlowFile::decode_content_from_packets(const std::vector<RawPacket>& packets) {
    std::vector<uint8_t> content_data;
    
    // Sort packets by sequence number
    std::vector<RawPacket> sorted_packets = packets;
    std::sort(sorted_packets.begin(), sorted_packets.end(),
              [](const RawPacket& a, const RawPacket& b) {
                  return a.header().sequence < b.header().sequence;
              });
    
    // Combine packet payloads
    for (const auto& packet : sorted_packets) {
        const auto& data = packet.data();
        content_data.insert(content_data.end(), data.begin(), data.end());
    }
    
    // Deserialize content
    deserialize_content(content_data);
}

std::vector<std::string> FlowFile::split_into_lines(const std::string& content) const {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

std::string FlowFile::join_lines(const std::vector<std::string>& lines) const {
    std::ostringstream stream;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            stream << '\n';
        }
        stream << lines[i];
    }
    
    return stream.str();
}

} // namespace nerd