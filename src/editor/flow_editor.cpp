#include "editor/flow_editor.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <fstream> // Added for file operations
#include <chrono>  // Added for timestamp
#include <cstdint> // Added for uint64_t

namespace nerd {

FlowEditor::FlowEditor() {
    flow_manager_ = std::make_unique<FlowManager>();
}

FlowEditor::~FlowEditor() = default;

bool FlowEditor::open_flow(const std::string& flow_name) {
    if (!flow_manager_) {
        set_error("Flow manager not initialized");
        return false;
    }
    
    FlowFile* flow = flow_manager_->open_flow(flow_name);
    if (!flow) {
        set_error("Failed to open flow: " + flow_name);
        return false;
    }
    
    state_.current_flow = flow;
    state_.current_line = 0;
    state_.is_modified = false;
    
    // Set up content change callback
    flow->set_content_change_callback([this](const std::string& new_content) {
        this->handle_content_change(new_content);
    });
    
    clear_error();
    std::cout << "Opened flow: " << flow_name << std::endl;
    return true;
}

void FlowEditor::close_flow() {
    if (state_.current_flow) {
        std::string flow_name = state_.current_flow->name();
        flow_manager_->close_flow(flow_name);
        state_.current_flow = nullptr;
        state_.current_line = 0;
        state_.is_modified = false;
        std::cout << "Closed flow: " << flow_name << std::endl;
    }
}

void FlowEditor::run_interactive() {
    std::cout << "NERD: Network-Flow Editor" << std::endl;
    std::cout << "Editing living data streams that exist as active network circulation patterns" << std::endl;
    std::cout << "Type 'help' for commands, 'quit' to exit" << std::endl;
    
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit" || input == "q") {
            break;
        }
        
        run_command(input);
        
        if (!state_.last_error.empty()) {
            std::cout << "Error: " << state_.last_error << std::endl;
            clear_error();
        }
    }
}

void FlowEditor::run_command(const std::string& command) {
    if (command.empty()) {
        return;
    }
    
    if (!execute_command(command)) {
        set_error("Invalid command: " + command);
    }
}

void FlowEditor::append_line(const std::string& line) {
    if (!state_.current_flow) {
        set_error("No flow open");
        return;
    }
    
    state_.current_flow->append_content(line);
    state_.is_modified = true;
    update_current_line();
}

void FlowEditor::delete_lines(int start, int end) {
    if (!state_.current_flow) {
        set_error("No flow open");
        return;
    }
    
    if (!validate_line_range(start, end)) {
        set_error("Invalid line range");
        return;
    }
    
    state_.current_flow->delete_content(start, end);
    state_.is_modified = true;
    update_current_line();
}

void FlowEditor::substitute_text(const std::string& pattern, const std::string& replacement) {
    if (!state_.current_flow) {
        set_error("No flow open");
        return;
    }
    
    state_.current_flow->substitute_content(pattern, replacement);
    state_.is_modified = true;
}

void FlowEditor::insert_line(int line, const std::string& content) {
    if (!state_.current_flow) {
        set_error("No flow open");
        return;
    }
    
    state_.current_flow->insert_content(line, content);
    state_.is_modified = true;
    update_current_line();
}

void FlowEditor::print_current_line() {
    if (!state_.current_flow) {
        set_error("No flow open");
        return;
    }
    
    std::vector<std::string> lines = split_lines(state_.current_flow->content());
    if (state_.current_line >= 0 && state_.current_line < static_cast<int>(lines.size())) {
        std::cout << (state_.current_line + 1) << "\t" << lines[state_.current_line] << std::endl;
    }
}

void FlowEditor::print_all_lines() {
    if (!state_.current_flow) {
        set_error("No flow open");
        return;
    }
    
    std::vector<std::string> lines = split_lines(state_.current_flow->content());
    print_lines(lines, 0, lines.size() - 1);
}

void FlowEditor::print_line_range(int start, int end) {
    if (!state_.current_flow) {
        set_error("No flow open");
        return;
    }
    
    if (!validate_line_range(start, end)) {
        set_error("Invalid line range");
        return;
    }
    
    std::vector<std::string> lines = split_lines(state_.current_flow->content());
    print_lines(lines, start, end);
}

void FlowEditor::write_flow() {
    if (!state_.current_flow) {
        set_error("No flow open");
        return;
    }
    
    write_pattern_changes();
    state_.is_modified = false;
    std::cout << "Flow written to circulation pattern" << std::endl;
}

void FlowEditor::quit_editor() {
    if (state_.is_modified) {
        std::cout << "Warning: Flow has unsaved changes" << std::endl;
    }
    close_flow();
}

bool FlowEditor::initialize_network(const std::string& interface) {
    if (!flow_manager_) {
        set_error("Flow manager not initialized");
        return false;
    }
    
    bool success = flow_manager_->initialize_network(interface);
    if (!success) {
        set_error("Failed to initialize network on interface: " + interface);
    }
    
    return success;
}

void FlowEditor::discover_flows() {
    std::cout << "Discovering existing flows in network..." << std::endl;
    
    if (flow_manager_) {
        std::vector<std::string> discovered = flow_manager_->discover_existing_flows();
        
        if (discovered.empty()) {
            std::cout << "No flows discovered in network." << std::endl;
            std::cout << "Network interfaces may be down or no other NERD instances are running." << std::endl;
        } else {
            std::cout << "Found flows:" << std::endl;
            for (const auto& flow : discovered) {
                std::cout << "  " << flow << std::endl;
            }
        }
    } else {
        std::cout << "Flow manager not initialized." << std::endl;
    }
    
    // Also show currently active flows
    std::cout << "\nCurrently active flows:" << std::endl;
    std::vector<std::string> active = get_available_flows();
    if (active.empty()) {
        std::cout << "  No flows currently open" << std::endl;
    } else {
        for (const auto& flow : active) {
            std::cout << "  " << flow << std::endl;
        }
    }
}

std::vector<std::string> FlowEditor::get_available_flows() const {
    if (flow_manager_) {
        return flow_manager_->get_active_flow_names();
    }
    return {};
}

std::string FlowEditor::parse_command(const std::string& input) {
    // Simple command parsing - in a real implementation, this would be more sophisticated
    return input;
}

bool FlowEditor::execute_command(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == "help" || cmd == "h") {
        std::cout << "Available commands:" << std::endl;
        std::cout << "  open <flow_name>  - Open a flow" << std::endl;
        std::cout << "  close             - Close current flow" << std::endl;
        std::cout << "  append <text>     - Append text to flow" << std::endl;
        std::cout << "  delete <start> <end> - Delete lines" << std::endl;
        std::cout << "  substitute <pattern> <replacement> - Substitute text" << std::endl;
        std::cout << "  print             - Print current line" << std::endl;
        std::cout << "  print all         - Print all lines" << std::endl;
        std::cout << "  write             - Write flow to circulation" << std::endl;
        std::cout << "  discover          - Discover existing flows in network" << std::endl;
        std::cout << "  list              - List currently available flows" << std::endl;
        std::cout << "  status            - Show current flow status" << std::endl;
        std::cout << "  stats             - Show flow statistics" << std::endl;
        std::cout << "  simulate          - Toggle simulation mode" << std::endl;
        std::cout << "  export <filename> - Export current flow to file" << std::endl;
        std::cout << "  import <filename> - Import flow from file" << std::endl;
        std::cout << "  monitor           - Monitor network activity" << std::endl;
        std::cout << "  quit              - Quit editor" << std::endl;
        return true;
    }
    
    if (cmd == "open") {
        std::string flow_name;
        iss >> flow_name;
        return open_flow(flow_name);
    }
    
    if (cmd == "close") {
        close_flow();
        return true;
    }
    
    if (cmd == "append" || cmd == "a") {
        std::string text;
        std::getline(iss, text);
        if (!text.empty() && text[0] == ' ') {
            text = text.substr(1);
        }
        append_line(text);
        return true;
    }
    
    if (cmd == "delete" || cmd == "d") {
        int start, end;
        iss >> start >> end;
        delete_lines(start - 1, end - 1); // Convert to 0-based
        return true;
    }
    
    if (cmd == "substitute" || cmd == "s") {
        std::string pattern, replacement;
        iss >> pattern >> replacement;
        substitute_text(pattern, replacement);
        return true;
    }
    
    if (cmd == "print" || cmd == "p") {
        std::string arg;
        iss >> arg;
        if (arg == "all") {
            print_all_lines();
        } else {
            print_current_line();
        }
        return true;
    }
    
    if (cmd == "write" || cmd == "w") {
        write_flow();
        return true;
    }
    
    if (cmd == "discover") {
        discover_flows();
        return true;
    }
    
    if (cmd == "list") {
        std::vector<std::string> flows = get_available_flows();
        std::cout << "Available flows:" << std::endl;
        if (flows.empty()) {
            std::cout << "  No flows currently active" << std::endl;
        } else {
            for (const auto& flow : flows) {
                std::cout << "  " << flow << std::endl;
            }
        }
        return true;
    }
    
    if (cmd == "status") {
        print_flow_state();
        return true;
    }
    
    if (cmd == "stats") {
        print_flow_statistics();
        return true;
    }
    
    if (cmd == "simulate") {
        toggle_simulation_mode();
        return true;
    }
    
    if (cmd == "export") {
        std::string filename;
        iss >> filename;
        if (filename.empty()) {
            std::cout << "Usage: export <filename>" << std::endl;
        } else {
            export_flow(filename);
        }
        return true;
    }
    
    if (cmd == "import") {
        std::string filename;
        iss >> filename;
        if (filename.empty()) {
            std::cout << "Usage: import <filename>" << std::endl;
        } else {
            import_flow(filename);
        }
        return true;
    }
    
    if (cmd == "monitor") {
        monitor_network_activity();
        return true;
    }
    
    if (cmd == "quit" || cmd == "q") {
        quit_editor();
        return true;
    }
    
    return false;
}

void FlowEditor::append_to_flow(const std::string& line) {
    append_line(line);
}

void FlowEditor::delete_from_flow(int start, int end) {
    delete_lines(start, end);
}

void FlowEditor::substitute_in_flow(const std::string& pattern, const std::string& replacement) {
    substitute_text(pattern, replacement);
}

void FlowEditor::print_flow_state() {
    if (!state_.current_flow) {
        std::cout << "No flow open" << std::endl;
        return;
    }
    
    std::cout << "Current flow: " << state_.current_flow->name() << std::endl;
    std::cout << "Current line: " << (state_.current_line + 1) << std::endl;
    std::cout << "Modified: " << (state_.is_modified ? "yes" : "no") << std::endl;
    
    std::vector<std::string> lines = split_lines(state_.current_flow->content());
    std::cout << "Total lines: " << lines.size() << std::endl;
}

void FlowEditor::write_pattern_changes() {
    if (!state_.current_flow) {
        return;
    }
    
    // In a real implementation, this would write the changes to the circulation pattern
    state_.current_flow->maintain_flow();
}

std::vector<std::string> FlowEditor::split_lines(const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

void FlowEditor::print_lines(const std::vector<std::string>& lines, int start, int end) {
    for (int i = start; i <= end && i < static_cast<int>(lines.size()); ++i) {
        std::cout << (i + 1) << "\t" << lines[i] << std::endl;
    }
}

bool FlowEditor::validate_line_range(int start, int end) {
    if (!state_.current_flow) {
        return false;
    }
    
    std::vector<std::string> lines = split_lines(state_.current_flow->content());
    return start >= 0 && start < static_cast<int>(lines.size()) &&
           end >= start && end < static_cast<int>(lines.size());
}

void FlowEditor::handle_content_change(const std::string& new_content) {
    state_.is_modified = true;
    update_current_line();
}

void FlowEditor::update_current_line() {
    if (!state_.current_flow) {
        return;
    }
    
    std::vector<std::string> lines = split_lines(state_.current_flow->content());
    if (state_.current_line >= static_cast<int>(lines.size())) {
        state_.current_line = std::max(0, static_cast<int>(lines.size()) - 1);
    }
}

void FlowEditor::set_error(const std::string& error) {
    state_.last_error = error;
}

void FlowEditor::clear_error() {
    state_.last_error.clear();
}

void FlowEditor::print_flow_statistics() {
    std::cout << "\n=== NERD Flow Statistics ===" << std::endl;
    
    if (!state_.current_flow) {
        std::cout << "No flow currently open" << std::endl;
    } else {
        std::cout << "Current Flow: " << state_.current_flow->name() << std::endl;
        std::cout << "Flow ID: " << state_.current_flow->identifier() << std::endl;
        
        std::string content = state_.current_flow->content();
        std::vector<std::string> lines = split_lines(content);
        
        std::cout << "Lines: " << lines.size() << std::endl;
        std::cout << "Characters: " << content.length() << std::endl;
        std::cout << "Words: " << count_words(content) << std::endl;
        std::cout << "Modified: " << (state_.is_modified ? "Yes" : "No") << std::endl;
        
        // Network flow statistics
        std::cout << "\nNetwork Status:" << std::endl;
        std::cout << "Simulation Mode: " << (state_.simulation_mode ? "On" : "Off") << std::endl;
        std::cout << "Flow Circulation: Active" << std::endl;
        std::cout << "Heartbeat Packets: Sending" << std::endl;
    }
    
    // Overall statistics
    std::vector<std::string> active_flows = get_available_flows();
    std::cout << "\nGlobal Statistics:" << std::endl;
    std::cout << "Active Flows: " << active_flows.size() << std::endl;
    std::cout << "Network Interface: " << (state_.simulation_mode ? "Simulated" : "Live") << std::endl;
    
    if (flow_manager_) {
        std::cout << "Flow Manager: Active" << std::endl;
    } else {
        std::cout << "Flow Manager: Inactive" << std::endl;
    }
}

void FlowEditor::toggle_simulation_mode() {
    state_.simulation_mode = !state_.simulation_mode;
    
    std::cout << "Simulation mode: " << (state_.simulation_mode ? "ON" : "OFF") << std::endl;
    
    if (state_.simulation_mode) {
        std::cout << "Running in simulation mode - network packets are simulated." << std::endl;
        std::cout << "No actual network traffic will be generated." << std::endl;
    } else {
        std::cout << "Live network mode - actual packets will be sent." << std::endl;
        std::cout << "Note: Raw socket access requires root privileges." << std::endl;
    }
}

size_t FlowEditor::count_words(const std::string& text) {
    std::istringstream iss(text);
    std::string word;
    size_t count = 0;
    
    while (iss >> word) {
        count++;
    }
    
    return count;
}

void FlowEditor::export_flow(const std::string& filename) {
    if (!state_.current_flow) {
        std::cout << "No flow currently open to export" << std::endl;
        return;
    }
    
    std::cout << "Exporting flow '" << state_.current_flow->name() << "' to '" << filename << "'..." << std::endl;
    
    try {
        std::ofstream outfile(filename);
        if (!outfile) {
            std::cout << "Error: Could not create file '" << filename << "'" << std::endl;
            return;
        }
        
        // Write flow metadata
        outfile << "# NERD Flow Export" << std::endl;
        outfile << "# Flow Name: " << state_.current_flow->name() << std::endl;
        outfile << "# Flow ID: " << state_.current_flow->identifier() << std::endl;
        outfile << "# Export Time: " << std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() << std::endl;
        outfile << "# --- Flow Content ---" << std::endl;
        
        // Write flow content
        outfile << state_.current_flow->content();
        
        outfile.close();
        std::cout << "Flow exported successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Export failed: " << e.what() << std::endl;
    }
}

void FlowEditor::import_flow(const std::string& filename) {
    std::cout << "Importing flow from '" << filename << "'..." << std::endl;
    
    try {
        std::ifstream infile(filename);
        if (!infile) {
            std::cout << "Error: Could not open file '" << filename << "'" << std::endl;
            return;
        }
        
        std::string line;
        std::string content;
        std::string flow_name;
        bool reading_content = false;
        
        // Parse file and extract content
        while (std::getline(infile, line)) {
            if (line.find("# Flow Name: ") == 0) {
                flow_name = line.substr(13); // Skip "# Flow Name: "
            } else if (line.find("# --- Flow Content ---") == 0) {
                reading_content = true;
                continue;
            } else if (reading_content) {
                if (!content.empty()) {
                    content += "\n";
                }
                content += line;
            }
        }
        
        infile.close();
        
        if (flow_name.empty()) {
            flow_name = "imported_flow";
        }
        
        // Create or open the flow
        if (open_flow(flow_name)) {
            // Clear existing content by setting it to empty string, then add imported content
            state_.current_flow->write_to_flow("");
            
            // Add content line by line
            std::istringstream content_stream(content);
            std::string content_line;
            while (std::getline(content_stream, content_line)) {
                state_.current_flow->append_content(content_line);
            }
            
            state_.is_modified = true;
            std::cout << "Flow imported successfully as '" << flow_name << "'" << std::endl;
        } else {
            std::cout << "Failed to create flow for import" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Import failed: " << e.what() << std::endl;
    }
}

void FlowEditor::monitor_network_activity() {
    std::cout << "\n=== Network Activity Monitor ===" << std::endl;
    std::cout << "Monitoring network flow activity..." << std::endl;
    
    if (state_.simulation_mode) {
        std::cout << "Running in simulation mode - showing simulated activity." << std::endl;
        
        // Simulate network activity for demonstration
        std::vector<std::string> activity_log = {
            "HEARTBEAT packet sent for flow 'testdoc' (ID: 1)",
            "DISCOVERY packet received from 192.168.1.100",
            "FLOW_DATA packet sent: 64 bytes",
            "Flow circulation maintained for 1 active flows",
            "Network interface: Simulated (eth0)",
            "Packet loss: 0%",
            "Average latency: 2ms",
            "Active connections: 1"
        };
        
        for (const auto& log_entry : activity_log) {
            std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count() % 100000 
                << "] " << log_entry << std::endl;
        }
        
    } else {
        std::cout << "Live network monitoring not yet implemented." << std::endl;
        std::cout << "This would show real-time packet flow for active flows." << std::endl;
    }
    
    std::cout << "\nFlow Status Summary:" << std::endl;
    std::vector<std::string> active_flows = get_available_flows();
    for (const auto& flow : active_flows) {
        std::cout << "  " << flow << ": Active circulation" << std::endl;
    }
    
    if (active_flows.empty()) {
        std::cout << "  No active flows to monitor" << std::endl;
    }
    
    std::cout << "=== End Monitor ===" << std::endl;
}

} // namespace nerd