#include "editor/flow_editor.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>

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
    if (flow_manager_) {
        flow_manager_->discover_existing_flows();
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
        std::cout << "  insert <line> <text> - Insert text at line" << std::endl;
        std::cout << "  delete <start> <end> - Delete lines" << std::endl;
        std::cout << "  substitute <pattern> <replacement> - Substitute text" << std::endl;
        std::cout << "  print             - Print current line" << std::endl;
        std::cout << "  print all         - Print all lines" << std::endl;
        std::cout << "  discover          - Discover existing flows" << std::endl;
        std::cout << "  list              - List active flows" << std::endl;
        std::cout << "  status            - Show current flow status" << std::endl;
        std::cout << "  write             - Write flow to circulation" << std::endl;
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
    
    if (cmd == "insert" || cmd == "i") {
        int line;
        iss >> line;
        std::string text;
        std::getline(iss, text);
        if (!text.empty() && text[0] == ' ') {
            text = text.substr(1);
        }
        insert_line(line - 1, text);
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
    
    if (cmd == "discover") {
        if (flow_manager_) {
            auto discovered = flow_manager_->discover_existing_flows();
            if (discovered.empty()) {
                std::cout << "No flows discovered" << std::endl;
            } else {
                std::cout << "Discovered flows:" << std::endl;
                for (const auto& name : discovered) {
                    std::cout << "  " << name << std::endl;
                }
            }
            return true;
        }
        return false;
    }
    
    if (cmd == "list" || cmd == "ls") {
        auto names = get_available_flows();
        if (names.empty()) {
            std::cout << "No active flows" << std::endl;
        } else {
            std::cout << "Active flows:" << std::endl;
            for (const auto& name : names) {
                std::cout << "  " << name << std::endl;
            }
        }
        return true;
    }
    
    if (cmd == "status") {
        print_flow_state();
        return true;
    }
    
    if (cmd == "write" || cmd == "w") {
        write_flow();
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

} // namespace nerd