#pragma once

#include "editor/flow_editor.h"
#include "workflow/workflow_manager.h"
#include "persistence/flow_persistence.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <iostream>
#include <sstream>

namespace nerd {

// CLI command categories
enum class CommandCategory {
    FLOW_OPS,           // Flow operations
    WORKFLOW_OPS,       // Workflow operations
    PERSISTENCE_OPS,    // Persistence operations
    SYSTEM_OPS,         // System operations
    HELP_OPS            // Help operations
};

// Command help information
struct CommandHelp {
    std::string command;
    std::string description;
    std::string usage;
    std::vector<std::string> examples;
    CommandCategory category;
    
    CommandHelp() = default;
    CommandHelp(const std::string& cmd, const std::string& desc, 
                const std::string& usage_str, CommandCategory cat)
        : command(cmd), description(desc), usage(usage_str), category(cat) {}
};

// CLI configuration
struct CLIConfig {
    bool color_output;
    bool verbose_mode;
    bool auto_save;
    std::string prompt;
    std::string log_file;
    size_t history_size;
    
    CLIConfig() : color_output(true), verbose_mode(false), auto_save(true),
                  prompt("nerd> "), history_size(1000) {}
};

// Enhanced CLI with better UX and error handling
class EnhancedCLI {
private:
    std::unique_ptr<FlowEditor> editor_;
    std::unique_ptr<WorkflowManager> workflow_manager_;
    std::unique_ptr<FlowPersistence> persistence_;
    std::unique_ptr<NetworkDisruptionHandler> disruption_handler_;
    
    CLIConfig config_;
    std::vector<std::string> command_history_;
    std::map<std::string, CommandHelp> command_help_;
    std::string current_flow_name_;
    bool is_running_;
    
    // Color codes for output
    static const std::string COLOR_RESET;
    static const std::string COLOR_RED;
    static const std::string COLOR_GREEN;
    static const std::string COLOR_YELLOW;
    static const std::string COLOR_BLUE;
    static const std::string COLOR_CYAN;
    static const std::string COLOR_MAGENTA;
    
public:
    EnhancedCLI();
    ~EnhancedCLI();
    
    // Main CLI interface
    void run();
    void run_interactive();
    void run_script(const std::string& script_file);
    
    // Configuration
    void set_config(const CLIConfig& config) { config_ = config; }
    CLIConfig& get_config() { return config_; }
    
    // Command processing
    bool process_command(const std::string& input);
    void add_custom_command(const std::string& name, 
                           std::function<void(const std::vector<std::string>&)> handler);
    
    // Help system
    void show_help();
    void show_help(const std::string& command);
    void show_help_category(CommandCategory category);
    void register_command_help(const CommandHelp& help);
    
    // Output formatting
    void print_success(const std::string& message);
    void print_error(const std::string& message);
    void print_warning(const std::string& message);
    void print_info(const std::string& message);
    void print_debug(const std::string& message);
    
    // Flow operations
    void cmd_open_flow(const std::vector<std::string>& args);
    void cmd_close_flow(const std::vector<std::string>& args);
    void cmd_list_flows(const std::vector<std::string>& args);
    void cmd_flow_status(const std::vector<std::string>& args);
    void cmd_flow_info(const std::vector<std::string>& args);
    
    // Workflow operations
    void cmd_workflow_list(const std::vector<std::string>& args);
    void cmd_workflow_run(const std::vector<std::string>& args);
    void cmd_workflow_enable(const std::vector<std::string>& args);
    void cmd_workflow_disable(const std::vector<std::string>& args);
    void cmd_workflow_create(const std::vector<std::string>& args);
    void cmd_cost_analysis(const std::vector<std::string>& args);
    
    // Persistence operations
    void cmd_save_flow(const std::vector<std::string>& args);
    void cmd_load_flow(const std::vector<std::string>& args);
    void cmd_backup_flow(const std::vector<std::string>& args);
    void cmd_restore_flow(const std::vector<std::string>& args);
    void cmd_checkpoint_create(const std::vector<std::string>& args);
    void cmd_checkpoint_list(const std::vector<std::string>& args);
    void cmd_checkpoint_restore(const std::vector<std::string>& args);
    
    // System operations
    void cmd_system_status(const std::vector<std::string>& args);
    void cmd_system_info(const std::vector<std::string>& args);
    void cmd_network_status(const std::vector<std::string>& args);
    void cmd_performance_stats(const std::vector<std::string>& args);
    void cmd_cleanup(const std::vector<std::string>& args);
    
    // Configuration commands
    void cmd_config_show(const std::vector<std::string>& args);
    void cmd_config_set(const std::vector<std::string>& args);
    void cmd_config_reset(const std::vector<std::string>& args);
    
    // Utility commands
    void cmd_clear(const std::vector<std::string>& args);
    void cmd_history(const std::vector<std::string>& args);
    void cmd_export(const std::vector<std::string>& args);
    void cmd_import(const std::vector<std::string>& args);
    
private:
    // Command parsing
    std::vector<std::string> parse_arguments(const std::string& input);
    std::string get_command_name(const std::vector<std::string>& args);
    std::vector<std::string> get_command_args(const std::vector<std::string>& args);
    
    // Command routing
    void route_command(const std::string& command, const std::vector<std::string>& args);
    void handle_unknown_command(const std::string& command);
    
    // Input processing
    std::string read_line();
    std::string read_multiline();
    bool is_multiline_command(const std::string& input);
    
    // History management
    void add_to_history(const std::string& command);
    void show_history();
    std::string get_history_item(int index);
    
    // Output helpers
    void print_prompt();
    void print_banner();
    void print_version();
    void print_usage();
    
    // Error handling
    void handle_error(const std::string& error);
    void handle_exception(const std::exception& e);
    
    // Initialization
    void initialize_components();
    void register_builtin_commands();
    void setup_workflows();
    void setup_persistence();
    
    // Auto-completion
    std::vector<std::string> get_command_completions(const std::string& prefix);
    std::vector<std::string> get_flow_completions(const std::string& prefix);
    std::vector<std::string> get_workflow_completions(const std::string& prefix);
    
    // Logging
    void log_command(const std::string& command);
    void log_error(const std::string& error);
    void log_info(const std::string& info);
};

} // namespace nerd