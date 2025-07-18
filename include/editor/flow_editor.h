#pragma once

#include "core/flow_file.h"
#include "network/flow_manager.h"
#include <string>
#include <memory>
#include <functional>

namespace nerd {

// Editor state
struct EditorState {
    FlowFile* current_flow;
    int current_line;
    bool is_modified;
    bool simulation_mode;
    std::string last_error;
    
    EditorState() : current_flow(nullptr), current_line(0), is_modified(false), simulation_mode(false) {}
};

// FlowEditor class - provides ed-compatible interface for editing flows
class FlowEditor {
private:
    std::unique_ptr<FlowManager> flow_manager_;
    EditorState state_;
    
    // Command parsing
    std::string parse_command(const std::string& input);
    bool execute_command(const std::string& command);
    
    // Ed-compatible commands
    void append_to_flow(const std::string& line);
    void delete_from_flow(int start, int end);
    void substitute_in_flow(const std::string& pattern, const std::string& replacement);
    void print_flow_state();
    void write_pattern_changes();
    
    // Helper functions
    std::vector<std::string> split_lines(const std::string& content);
    void print_lines(const std::vector<std::string>& lines, int start, int end);
    bool validate_line_range(int start, int end);
    
public:
    FlowEditor();
    ~FlowEditor();
    
    // Editor interface
    bool open_flow(const std::string& flow_name);
    void close_flow();
    void run_interactive();
    void run_command(const std::string& command);
    
    // Flow operations
    void append_line(const std::string& line);
    void delete_lines(int start, int end);
    void substitute_text(const std::string& pattern, const std::string& replacement);
    void insert_line(int line, const std::string& content);
    
    // Display operations
    void print_current_line();
    void print_all_lines();
    void print_line_range(int start, int end);
    
    // File operations
    void write_flow();
    void quit_editor();
    
    // State access
    const EditorState& state() const { return state_; }
    FlowFile* current_flow() const { return state_.current_flow; }
    bool has_current_flow() const { return state_.current_flow != nullptr; }
    
    // Network management
    bool initialize_network(const std::string& interface);
    void discover_flows();
    std::vector<std::string> get_available_flows() const;
    
    // Enhanced features
    void print_flow_statistics();
    void toggle_simulation_mode();
    void export_flow(const std::string& filename);
    void import_flow(const std::string& filename);
    void monitor_network_activity();
    
private:
    void handle_content_change(const std::string& new_content);
    void update_current_line();
    void set_error(const std::string& error);
    void clear_error();
    size_t count_words(const std::string& text);
};

} // namespace nerd