#pragma once

#include "network/packet.h"
#include "network/flow.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace nerd {

// Edit command for modifying flow patterns
struct EditCommand {
    enum Type {
        APPEND,
        DELETE,
        SUBSTITUTE,
        INSERT
    };
    
    Type type;
    std::string data;
    int start_line;
    int end_line;
    std::string pattern;
    std::string replacement;
    
    EditCommand() : type(APPEND), start_line(0), end_line(0) {}
};

// FlowFile class - represents a file as a living network process
class FlowFile {
private:
    FlowID identifier_;
    CirculationPattern pattern_;
    std::vector<NetworkNode> circulation_path_;
    std::string name_;
    std::string content_;
    bool is_modified_;
    
    // Callback for content changes
    std::function<void(const std::string&)> content_change_callback_;
    
public:
    FlowFile(FlowID id, const std::string& name);
    ~FlowFile();
    
    // File operations
    void maintain_flow();
    void modify_pattern(const EditCommand& cmd);
    std::string read_from_flow();
    void write_to_flow(const std::string& data);
    
    // Content management
    void append_content(const std::string& line);
    void delete_content(int start_line, int end_line);
    void substitute_content(const std::string& pattern, const std::string& replacement);
    void insert_content(int line, const std::string& content);
    
    // Flow state
    void update_circulation_pattern(const CirculationPattern& pattern);
    void add_circulation_node(const NetworkNode& node);
    void remove_circulation_node(const std::string& address);
    
    // Accessors
    FlowID identifier() const { return identifier_; }
    const std::string& name() const { return name_; }
    const std::string& content() const { return content_; }
    const CirculationPattern& pattern() const { return pattern_; }
    const std::vector<NetworkNode>& circulation_path() const { return circulation_path_; }
    bool is_modified() const { return is_modified_; }
    
    // Callbacks
    void set_content_change_callback(std::function<void(const std::string&)> callback) {
        content_change_callback_ = callback;
    }
    
    // Serialization
    std::vector<uint8_t> serialize_content() const;
    bool deserialize_content(const std::vector<uint8_t>& data);
    
    // Flow discovery
    bool discover_existing_flow();
    void broadcast_existence();
    
private:
    void notify_content_change();
    void encode_content_in_packets();
    void decode_content_from_packets(const std::vector<RawPacket>& packets);
    std::vector<std::string> split_into_lines(const std::string& content) const;
    std::string join_lines(const std::vector<std::string>& lines) const;
};

} // namespace nerd