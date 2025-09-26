#include "cli/enhanced_cli.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <thread>

namespace nerd {

// Color constants
const std::string EnhancedCLI::COLOR_RESET = "\033[0m";
const std::string EnhancedCLI::COLOR_RED = "\033[31m";
const std::string EnhancedCLI::COLOR_GREEN = "\033[32m";
const std::string EnhancedCLI::COLOR_YELLOW = "\033[33m";
const std::string EnhancedCLI::COLOR_BLUE = "\033[34m";
const std::string EnhancedCLI::COLOR_CYAN = "\033[36m";
const std::string EnhancedCLI::COLOR_MAGENTA = "\033[35m";

EnhancedCLI::EnhancedCLI() : is_running_(false) {
    initialize_components();
    register_builtin_commands();
    setup_workflows();
    setup_persistence();
}

EnhancedCLI::~EnhancedCLI() = default;

void EnhancedCLI::run() {
    print_banner();
    print_version();
    print_info("NERD: Network-Flow Editor - Enhanced CLI");
    print_info("Type 'help' for available commands, 'quit' to exit");
    
    is_running_ = true;
    run_interactive();
}

void EnhancedCLI::run_interactive() {
    std::string input;
    
    while (is_running_) {
        try {
            print_prompt();
            input = read_line();
            
            if (input.empty()) {
                continue;
            }
            
            add_to_history(input);
            log_command(input);
            
            if (!process_command(input)) {
                print_error("Unknown command: " + input);
            }
            
        } catch (const std::exception& e) {
            handle_exception(e);
        } catch (...) {
            print_error("Unknown error occurred");
        }
    }
}

void EnhancedCLI::run_script(const std::string& script_file) {
    std::ifstream file(script_file);
    if (!file.is_open()) {
        print_error("Failed to open script file: " + script_file);
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }
        
        print_info("Executing: " + line);
        process_command(line);
    }
    
    file.close();
    print_success("Script execution completed");
}

bool EnhancedCLI::process_command(const std::string& input) {
    std::vector<std::string> args = parse_arguments(input);
    if (args.empty()) {
        return true;
    }
    
    std::string command = get_command_name(args);
    std::vector<std::string> command_args = get_command_args(args);
    
    route_command(command, command_args);
    return true;
}

void EnhancedCLI::add_custom_command(const std::string& name, 
                                    std::function<void(const std::vector<std::string>&)> handler) {
    // In a real implementation, this would store custom commands
    print_info("Custom command added: " + name);
}

void EnhancedCLI::show_help() {
    print_info("Available commands:");
    print_info("");
    
    for (const auto& pair : command_help_) {
        const CommandHelp& help = pair.second;
        std::string category_str;
        
        switch (help.category) {
            case CommandCategory::FLOW_OPS:
                category_str = "Flow Operations";
                break;
            case CommandCategory::WORKFLOW_OPS:
                category_str = "Workflow Operations";
                break;
            case CommandCategory::PERSISTENCE_OPS:
                category_str = "Persistence Operations";
                break;
            case CommandCategory::SYSTEM_OPS:
                category_str = "System Operations";
                break;
            case CommandCategory::HELP_OPS:
                category_str = "Help Operations";
                break;
        }
        
        print_info("  " + help.command + " - " + help.description + " [" + category_str + "]");
    }
    
    print_info("");
    print_info("Use 'help <command>' for detailed information about a specific command");
}

void EnhancedCLI::show_help(const std::string& command) {
    auto it = command_help_.find(command);
    if (it != command_help_.end()) {
        const CommandHelp& help = it->second;
        
        print_info("Command: " + help.command);
        print_info("Description: " + help.description);
        print_info("Usage: " + help.usage);
        
        if (!help.examples.empty()) {
            print_info("Examples:");
            for (const auto& example : help.examples) {
                print_info("  " + example);
            }
        }
    } else {
        print_error("No help available for command: " + command);
    }
}

void EnhancedCLI::show_help_category(CommandCategory category) {
    print_info("Commands in category:");
    
    for (const auto& pair : command_help_) {
        if (pair.second.category == category) {
            print_info("  " + pair.second.command + " - " + pair.second.description);
        }
    }
}

void EnhancedCLI::register_command_help(const CommandHelp& help) {
    command_help_[help.command] = help;
}

void EnhancedCLI::print_success(const std::string& message) {
    if (config_.color_output) {
        std::cout << COLOR_GREEN << "✓ " << message << COLOR_RESET << std::endl;
    } else {
        std::cout << "SUCCESS: " << message << std::endl;
    }
}

void EnhancedCLI::print_error(const std::string& message) {
    if (config_.color_output) {
        std::cerr << COLOR_RED << "✗ " << message << COLOR_RESET << std::endl;
    } else {
        std::cerr << "ERROR: " << message << std::endl;
    }
    log_error(message);
}

void EnhancedCLI::print_warning(const std::string& message) {
    if (config_.color_output) {
        std::cout << COLOR_YELLOW << "⚠ " << message << COLOR_RESET << std::endl;
    } else {
        std::cout << "WARNING: " << message << std::endl;
    }
}

void EnhancedCLI::print_info(const std::string& message) {
    if (config_.color_output) {
        std::cout << COLOR_CYAN << "ℹ " << message << COLOR_RESET << std::endl;
    } else {
        std::cout << "INFO: " << message << std::endl;
    }
}

void EnhancedCLI::print_debug(const std::string& message) {
    if (config_.verbose_mode) {
        if (config_.color_output) {
            std::cout << COLOR_MAGENTA << "DEBUG: " << message << COLOR_RESET << std::endl;
        } else {
            std::cout << "DEBUG: " << message << std::endl;
        }
    }
    log_info(message);
}

// Flow operations
void EnhancedCLI::cmd_open_flow(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: open <flow_name>");
        return;
    }
    
    std::string flow_name = args[0];
    
    if (editor_->open_flow(flow_name)) {
        current_flow_name_ = flow_name;
        print_success("Opened flow: " + flow_name);
    } else {
        print_error("Failed to open flow: " + flow_name);
    }
}

void EnhancedCLI::cmd_close_flow(const std::vector<std::string>& args) {
    if (current_flow_name_.empty()) {
        print_warning("No flow currently open");
        return;
    }
    
    editor_->close_flow();
    print_success("Closed flow: " + current_flow_name_);
    current_flow_name_.clear();
}

void EnhancedCLI::cmd_list_flows(const std::vector<std::string>& args) {
    auto flows = editor_->get_available_flows();
    
    if (flows.empty()) {
        print_info("No active flows");
        return;
    }
    
    print_info("Active flows:");
    for (const auto& flow_name : flows) {
        std::string marker = (flow_name == current_flow_name_) ? " *" : "";
        print_info("  " + flow_name + marker);
    }
}

void EnhancedCLI::cmd_flow_status(const std::vector<std::string>& args) {
    if (!editor_->has_current_flow()) {
        print_warning("No flow currently open");
        return;
    }
    
    const auto& state = editor_->state();
    print_info("Current flow: " + current_flow_name_);
    print_info("Current line: " + std::to_string(state.current_line + 1));
    print_info("Modified: " + std::string(state.is_modified ? "yes" : "no"));
    
    if (!state.last_error.empty()) {
        print_error("Last error: " + state.last_error);
    }
}

void EnhancedCLI::cmd_flow_info(const std::vector<std::string>& args) {
    if (!editor_->has_current_flow()) {
        print_warning("No flow currently open");
        return;
    }
    
    FlowFile* flow = editor_->current_flow();
    print_info("Flow Information:");
    print_info("  Name: " + flow->name());
    print_info("  ID: " + std::to_string(flow->identifier()));
    print_info("  Content size: " + std::to_string(flow->content().length()) + " bytes");
    print_info("  Circulation rate: " + std::to_string(flow->pattern().circulation_rate) + " pps");
    print_info("  Max packet age: " + std::to_string(flow->pattern().max_packet_age) + " μs");
    print_info("  Auto-sustain: " + std::string(flow->pattern().auto_sustain ? "yes" : "no"));
}

// Workflow operations
void EnhancedCLI::cmd_workflow_list(const std::vector<std::string>& args) {
    auto workflows = workflow_manager_->list_workflows();
    
    if (workflows.empty()) {
        print_info("No workflows configured");
        return;
    }
    
    print_info("Available workflows:");
    for (const auto& workflow_name : workflows) {
        Workflow* workflow = workflow_manager_->get_workflow(workflow_name);
        if (workflow) {
            std::string status = workflow->enabled ? "enabled" : "disabled";
            print_info("  " + workflow_name + " (" + status + ")");
        }
    }
}

void EnhancedCLI::cmd_workflow_run(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: workflow run <workflow_name>");
        return;
    }
    
    std::string workflow_name = args[0];
    WorkflowContext context;
    
    if (editor_->has_current_flow()) {
        context.flow = editor_->current_flow();
        context.cost_info = workflow_manager_->get_flow_cost(editor_->current_flow()->identifier());
    }
    
    workflow_manager_->trigger_workflow(workflow_name, context);
    print_success("Executed workflow: " + workflow_name);
}

void EnhancedCLI::cmd_workflow_enable(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: workflow enable <workflow_name>");
        return;
    }
    
    std::string workflow_name = args[0];
    workflow_manager_->enable_workflow(workflow_name, true);
    print_success("Enabled workflow: " + workflow_name);
}

void EnhancedCLI::cmd_workflow_disable(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: workflow disable <workflow_name>");
        return;
    }
    
    std::string workflow_name = args[0];
    workflow_manager_->enable_workflow(workflow_name, false);
    print_success("Disabled workflow: " + workflow_name);
}

void EnhancedCLI::cmd_workflow_create(const std::vector<std::string>& args) {
    print_info("Workflow creation not yet implemented");
    print_info("Use built-in workflows for now");
}

void EnhancedCLI::cmd_cost_analysis(const std::vector<std::string>& args) {
    WorkflowContext context;
    workflow_manager_->trigger_workflow("cost_analysis", context);
}

// Persistence operations
void EnhancedCLI::cmd_save_flow(const std::vector<std::string>& args) {
    if (!editor_->has_current_flow()) {
        print_warning("No flow currently open");
        return;
    }
    
    FlowFile* flow = editor_->current_flow();
    if (persistence_->save_flow(*flow)) {
        print_success("Flow saved to persistence: " + flow->name());
    } else {
        print_error("Failed to save flow to persistence");
    }
}

void EnhancedCLI::cmd_load_flow(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: load <flow_id>");
        return;
    }
    
    FlowID flow_id = std::stoull(args[0]);
    
    // Create a new flow file
    FlowFile flow(flow_id, "loaded_flow");
    
    if (persistence_->load_flow(flow_id, flow)) {
        print_success("Flow loaded from persistence: " + std::to_string(flow_id));
        // In a real implementation, this would open the loaded flow in the editor
    } else {
        print_error("Failed to load flow from persistence");
    }
}

void EnhancedCLI::cmd_backup_flow(const std::vector<std::string>& args) {
    if (!editor_->has_current_flow()) {
        print_warning("No flow currently open");
        return;
    }
    
    FlowFile* flow = editor_->current_flow();
    if (persistence_->create_snapshot(*flow)) {
        print_success("Flow backup created: " + flow->name());
    } else {
        print_error("Failed to create flow backup");
    }
}

void EnhancedCLI::cmd_restore_flow(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: restore <flow_id>");
        return;
    }
    
    FlowID flow_id = std::stoull(args[0]);
    
    if (editor_->has_current_flow()) {
        FlowFile* flow = editor_->current_flow();
        if (persistence_->restore_from_snapshot(flow_id, *flow)) {
            print_success("Flow restored from snapshot: " + std::to_string(flow_id));
        } else {
            print_error("Failed to restore flow from snapshot");
        }
    } else {
        print_error("No flow currently open");
    }
}

void EnhancedCLI::cmd_checkpoint_create(const std::vector<std::string>& args) {
    if (persistence_->create_checkpoint()) {
        print_success("Checkpoint created successfully");
    } else {
        print_error("Failed to create checkpoint");
    }
}

void EnhancedCLI::cmd_checkpoint_list(const std::vector<std::string>& args) {
    auto checkpoints = persistence_->list_checkpoints();
    
    if (checkpoints.empty()) {
        print_info("No checkpoints available");
        return;
    }
    
    print_info("Available checkpoints:");
    for (const auto& checkpoint_id : checkpoints) {
        print_info("  " + checkpoint_id);
    }
}

void EnhancedCLI::cmd_checkpoint_restore(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: checkpoint restore <checkpoint_id>");
        return;
    }
    
    std::string checkpoint_id = args[0];
    if (persistence_->restore_from_checkpoint(checkpoint_id)) {
        print_success("Restored from checkpoint: " + checkpoint_id);
    } else {
        print_error("Failed to restore from checkpoint");
    }
}

// System operations
void EnhancedCLI::cmd_system_status(const std::vector<std::string>& args) {
    print_info("System Status:");
    print_info("  Workflow engine: " + std::string(workflow_manager_->is_running() ? "running" : "stopped"));
    print_info("  Persistence: " + std::string(persistence_ ? "available" : "unavailable"));
    print_info("  Current flow: " + (current_flow_name_.empty() ? "none" : current_flow_name_));
    print_info("  Total cost: $" + std::to_string(workflow_manager_->get_total_cost()));
}

void EnhancedCLI::cmd_system_info(const std::vector<std::string>& args) {
    print_info("System Information:");
    print_info("  NERD Version: 0.1.0");
    print_info("  CLI Version: Enhanced 1.0");
    print_info("  Persistence directory: /tmp/nerd_persistence");
    print_info("  Color output: " + std::string(config_.color_output ? "enabled" : "disabled"));
    print_info("  Verbose mode: " + std::string(config_.verbose_mode ? "enabled" : "disabled"));
    print_info("  Auto-save: " + std::string(config_.auto_save ? "enabled" : "disabled"));
}

void EnhancedCLI::cmd_network_status(const std::vector<std::string>& args) {
    print_info("Network Status:");
    print_info("  Interface: eth0 (simulated)");
    print_info("  Raw socket: " + std::string(editor_->initialize_network("eth0") ? "available" : "unavailable"));
    print_info("  Disruption monitoring: " + std::string(disruption_handler_ ? "active" : "inactive"));
}

void EnhancedCLI::cmd_performance_stats(const std::vector<std::string>& args) {
    print_info("Performance Statistics:");
    print_info("  Active flows: " + std::to_string(editor_->get_available_flows().size()));
    print_info("  Persistence size: " + std::to_string(persistence_->get_persistence_size()) + " bytes");
    print_info("  Command history: " + std::to_string(command_history_.size()) + " commands");
}

void EnhancedCLI::cmd_cleanup(const std::vector<std::string>& args) {
    print_info("Performing cleanup...");
    
    // Cleanup old snapshots
    persistence_->cleanup_old_snapshots();
    
    // Compact persistence
    persistence_->compact_persistence();
    
    print_success("Cleanup completed");
}

// Configuration commands
void EnhancedCLI::cmd_config_show(const std::vector<std::string>& args) {
    print_info("Current Configuration:");
    print_info("  Color output: " + std::string(config_.color_output ? "enabled" : "disabled"));
    print_info("  Verbose mode: " + std::string(config_.verbose_mode ? "enabled" : "disabled"));
    print_info("  Auto-save: " + std::string(config_.auto_save ? "enabled" : "disabled"));
    print_info("  Prompt: " + config_.prompt);
    print_info("  History size: " + std::to_string(config_.history_size));
}

void EnhancedCLI::cmd_config_set(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        print_error("Usage: config set <key> <value>");
        return;
    }
    
    std::string key = args[0];
    std::string value = args[1];
    
    if (key == "color_output") {
        config_.color_output = (value == "true" || value == "1");
        print_success("Color output " + std::string(config_.color_output ? "enabled" : "disabled"));
    } else if (key == "verbose_mode") {
        config_.verbose_mode = (value == "true" || value == "1");
        print_success("Verbose mode " + std::string(config_.verbose_mode ? "enabled" : "disabled"));
    } else if (key == "auto_save") {
        config_.auto_save = (value == "true" || value == "1");
        print_success("Auto-save " + std::string(config_.auto_save ? "enabled" : "disabled"));
    } else if (key == "prompt") {
        config_.prompt = value;
        print_success("Prompt set to: " + value);
    } else {
        print_error("Unknown configuration key: " + key);
    }
}

void EnhancedCLI::cmd_config_reset(const std::vector<std::string>& args) {
    config_ = CLIConfig();
    print_success("Configuration reset to defaults");
}

// Utility commands
void EnhancedCLI::cmd_clear(const std::vector<std::string>& args) {
    // Clear screen (simplified)
    std::cout << "\033[2J\033[H";
    print_banner();
}

void EnhancedCLI::cmd_history(const std::vector<std::string>& args) {
    if (command_history_.empty()) {
        print_info("No command history");
        return;
    }
    
    print_info("Command History:");
    for (size_t i = 0; i < command_history_.size(); ++i) {
        print_info("  " + std::to_string(i + 1) + ": " + command_history_[i]);
    }
}

void EnhancedCLI::cmd_export(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: export <filename>");
        return;
    }
    
    std::string filename = args[0];
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        print_error("Failed to open file for export: " + filename);
        return;
    }
    
    // Export current flow if open
    if (editor_->has_current_flow()) {
        FlowFile* flow = editor_->current_flow();
        file << "Flow: " << flow->name() << std::endl;
        file << "Content:" << std::endl;
        file << flow->content() << std::endl;
    }
    
    file.close();
    print_success("Exported to: " + filename);
}

void EnhancedCLI::cmd_import(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("Usage: import <filename>");
        return;
    }
    
    std::string filename = args[0];
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        print_error("Failed to open file for import: " + filename);
        return;
    }
    
    // Simple import - just read the file content
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    print_success("Imported from: " + filename);
    print_info("Content length: " + std::to_string(content.length()) + " bytes");
}

// Private methods
std::vector<std::string> EnhancedCLI::parse_arguments(const std::string& input) {
    std::vector<std::string> args;
    std::istringstream iss(input);
    std::string arg;
    
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    return args;
}

std::string EnhancedCLI::get_command_name(const std::vector<std::string>& args) {
    return args.empty() ? "" : args[0];
}

std::vector<std::string> EnhancedCLI::get_command_args(const std::vector<std::string>& args) {
    if (args.size() <= 1) {
        return {};
    }
    
    return std::vector<std::string>(args.begin() + 1, args.end());
}

void EnhancedCLI::route_command(const std::string& command, const std::vector<std::string>& args) {
    if (command == "help" || command == "h") {
        if (args.empty()) {
            show_help();
        } else {
            show_help(args[0]);
        }
    } else if (command == "quit" || command == "q" || command == "exit") {
        is_running_ = false;
        print_success("Goodbye!");
    } else if (command == "open") {
        cmd_open_flow(args);
    } else if (command == "close") {
        cmd_close_flow(args);
    } else if (command == "list" || command == "ls") {
        cmd_list_flows(args);
    } else if (command == "status") {
        cmd_flow_status(args);
    } else if (command == "info") {
        cmd_flow_info(args);
    } else if (command == "workflow") {
        if (!args.empty()) {
            std::string subcommand = args[0];
            std::vector<std::string> subargs(args.begin() + 1, args.end());
            
            if (subcommand == "list") {
                cmd_workflow_list(subargs);
            } else if (subcommand == "run") {
                cmd_workflow_run(subargs);
            } else if (subcommand == "enable") {
                cmd_workflow_enable(subargs);
            } else if (subcommand == "disable") {
                cmd_workflow_disable(subargs);
            } else if (subcommand == "create") {
                cmd_workflow_create(subargs);
            } else {
                print_error("Unknown workflow command: " + subcommand);
            }
        } else {
            cmd_workflow_list(args);
        }
    } else if (command == "cost") {
        cmd_cost_analysis(args);
    } else if (command == "save") {
        cmd_save_flow(args);
    } else if (command == "load") {
        cmd_load_flow(args);
    } else if (command == "backup") {
        cmd_backup_flow(args);
    } else if (command == "restore") {
        cmd_restore_flow(args);
    } else if (command == "checkpoint") {
        if (!args.empty()) {
            std::string subcommand = args[0];
            std::vector<std::string> subargs(args.begin() + 1, args.end());
            
            if (subcommand == "create") {
                cmd_checkpoint_create(subargs);
            } else if (subcommand == "list") {
                cmd_checkpoint_list(subargs);
            } else if (subcommand == "restore") {
                cmd_checkpoint_restore(subargs);
            } else {
                print_error("Unknown checkpoint command: " + subcommand);
            }
        } else {
            cmd_checkpoint_list(args);
        }
    } else if (command == "system") {
        if (!args.empty()) {
            std::string subcommand = args[0];
            std::vector<std::string> subargs(args.begin() + 1, args.end());
            
            if (subcommand == "status") {
                cmd_system_status(subargs);
            } else if (subcommand == "info") {
                cmd_system_info(subargs);
            } else if (subcommand == "network") {
                cmd_network_status(subargs);
            } else if (subcommand == "performance") {
                cmd_performance_stats(subargs);
            } else if (subcommand == "cleanup") {
                cmd_cleanup(subargs);
            } else {
                print_error("Unknown system command: " + subcommand);
            }
        } else {
            cmd_system_status(args);
        }
    } else if (command == "config") {
        if (!args.empty()) {
            std::string subcommand = args[0];
            std::vector<std::string> subargs(args.begin() + 1, args.end());
            
            if (subcommand == "show") {
                cmd_config_show(subargs);
            } else if (subcommand == "set") {
                cmd_config_set(subargs);
            } else if (subcommand == "reset") {
                cmd_config_reset(subargs);
            } else {
                print_error("Unknown config command: " + subcommand);
            }
        } else {
            cmd_config_show(args);
        }
    } else if (command == "clear") {
        cmd_clear(args);
    } else if (command == "history") {
        cmd_history(args);
    } else if (command == "export") {
        cmd_export(args);
    } else if (command == "import") {
        cmd_import(args);
    } else {
        // Try to route to editor commands
        std::string args_str;
        for (const auto& arg : args) {
            if (!args_str.empty()) args_str += " ";
            args_str += arg;
        }
        editor_->run_command(command + " " + args_str);
    }
}

void EnhancedCLI::handle_unknown_command(const std::string& command) {
    print_error("Unknown command: " + command);
    print_info("Type 'help' for available commands");
}

std::string EnhancedCLI::read_line() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::string EnhancedCLI::read_multiline() {
    std::string input;
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line == "end" || line == ".") {
            break;
        }
        input += line + "\n";
    }
    
    return input;
}

bool EnhancedCLI::is_multiline_command(const std::string& input) {
    return input == "append" || input == "insert" || input == "substitute";
}

void EnhancedCLI::add_to_history(const std::string& command) {
    command_history_.push_back(command);
    
    // Limit history size
    if (command_history_.size() > config_.history_size) {
        command_history_.erase(command_history_.begin());
    }
}

void EnhancedCLI::show_history() {
    cmd_history({});
}

std::string EnhancedCLI::get_history_item(int index) {
    if (index >= 0 && index < static_cast<int>(command_history_.size())) {
        return command_history_[index];
    }
    return "";
}

void EnhancedCLI::print_prompt() {
    std::cout << config_.prompt;
    std::cout.flush();
}

void EnhancedCLI::print_banner() {
    if (config_.color_output) {
        std::cout << COLOR_CYAN << R"(
╔══════════════════════════════════════════════════════════════╗
║                    NERD: Network-Flow Editor                ║
║              Editing living data streams that exist as      ║
║              active network circulation patterns            ║
╚══════════════════════════════════════════════════════════════╝
)" << COLOR_RESET << std::endl;
    } else {
        std::cout << R"(
===============================================================
                    NERD: Network-Flow Editor
              Editing living data streams that exist as
              active network circulation patterns
===============================================================
)" << std::endl;
    }
}

void EnhancedCLI::print_version() {
    print_info("Version: 0.1.0 Enhanced CLI");
}

void EnhancedCLI::print_usage() {
    print_info("Usage: nerd [OPTIONS] [FLOW_NAME]");
    print_info("Options:");
    print_info("  -h, --help     Show this help message");
    print_info("  -v, --version  Show version information");
    print_info("  -i, --interface <interface>  Network interface to use");
}

void EnhancedCLI::handle_error(const std::string& error) {
    print_error(error);
}

void EnhancedCLI::handle_exception(const std::exception& e) {
    print_error("Exception: " + std::string(e.what()));
}

void EnhancedCLI::initialize_components() {
    editor_ = std::make_unique<FlowEditor>();
    workflow_manager_ = std::make_unique<WorkflowManager>();
    persistence_ = std::make_unique<FlowPersistence>();
    disruption_handler_ = std::make_unique<NetworkDisruptionHandler>(persistence_.get());
}

void EnhancedCLI::register_builtin_commands() {
    // Flow operations
    register_command_help(CommandHelp("open", "Open a flow", "open <flow_name>", CommandCategory::FLOW_OPS));
    register_command_help(CommandHelp("close", "Close current flow", "close", CommandCategory::FLOW_OPS));
    register_command_help(CommandHelp("list", "List active flows", "list", CommandCategory::FLOW_OPS));
    register_command_help(CommandHelp("status", "Show flow status", "status", CommandCategory::FLOW_OPS));
    register_command_help(CommandHelp("info", "Show flow information", "info", CommandCategory::FLOW_OPS));
    
    // Workflow operations
    register_command_help(CommandHelp("workflow", "Workflow operations", "workflow <subcommand>", CommandCategory::WORKFLOW_OPS));
    register_command_help(CommandHelp("cost", "Show cost analysis", "cost", CommandCategory::WORKFLOW_OPS));
    
    // Persistence operations
    register_command_help(CommandHelp("save", "Save flow to persistence", "save", CommandCategory::PERSISTENCE_OPS));
    register_command_help(CommandHelp("load", "Load flow from persistence", "load <flow_id>", CommandCategory::PERSISTENCE_OPS));
    register_command_help(CommandHelp("backup", "Create flow backup", "backup", CommandCategory::PERSISTENCE_OPS));
    register_command_help(CommandHelp("restore", "Restore flow from backup", "restore <flow_id>", CommandCategory::PERSISTENCE_OPS));
    register_command_help(CommandHelp("checkpoint", "Checkpoint operations", "checkpoint <subcommand>", CommandCategory::PERSISTENCE_OPS));
    
    // System operations
    register_command_help(CommandHelp("system", "System operations", "system <subcommand>", CommandCategory::SYSTEM_OPS));
    register_command_help(CommandHelp("config", "Configuration operations", "config <subcommand>", CommandCategory::SYSTEM_OPS));
    
    // Utility commands
    register_command_help(CommandHelp("clear", "Clear screen", "clear", CommandCategory::SYSTEM_OPS));
    register_command_help(CommandHelp("history", "Show command history", "history", CommandCategory::SYSTEM_OPS));
    register_command_help(CommandHelp("export", "Export current flow", "export <filename>", CommandCategory::SYSTEM_OPS));
    register_command_help(CommandHelp("import", "Import flow from file", "import <filename>", CommandCategory::SYSTEM_OPS));
    
    // Help
    register_command_help(CommandHelp("help", "Show help", "help [command]", CommandCategory::HELP_OPS));
    register_command_help(CommandHelp("quit", "Exit CLI", "quit", CommandCategory::HELP_OPS));
}

void EnhancedCLI::setup_workflows() {
    workflow_manager_->create_default_workflows();
    workflow_manager_->start_workflow_engine();
}

void EnhancedCLI::setup_persistence() {
    // Persistence is already initialized in constructor
    print_debug("Persistence system initialized");
}

std::vector<std::string> EnhancedCLI::get_command_completions(const std::string& prefix) {
    std::vector<std::string> completions;
    
    for (const auto& pair : command_help_) {
        if (pair.first.find(prefix) == 0) {
            completions.push_back(pair.first);
        }
    }
    
    return completions;
}

std::vector<std::string> EnhancedCLI::get_flow_completions(const std::string& prefix) {
    return editor_->get_available_flows();
}

std::vector<std::string> EnhancedCLI::get_workflow_completions(const std::string& prefix) {
    return workflow_manager_->list_workflows();
}

void EnhancedCLI::log_command(const std::string& command) {
    if (!config_.log_file.empty()) {
        std::ofstream log(config_.log_file, std::ios::app);
        if (log.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            log << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
                << " COMMAND: " << command << std::endl;
        }
    }
}

void EnhancedCLI::log_error(const std::string& error) {
    if (!config_.log_file.empty()) {
        std::ofstream log(config_.log_file, std::ios::app);
        if (log.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            log << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
                << " ERROR: " << error << std::endl;
        }
    }
}

void EnhancedCLI::log_info(const std::string& info) {
    if (!config_.log_file.empty()) {
        std::ofstream log(config_.log_file, std::ios::app);
        if (log.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            log << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
                << " INFO: " << info << std::endl;
        }
    }
}

} // namespace nerd