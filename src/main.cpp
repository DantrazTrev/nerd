#include "editor/flow_editor.h"
#include "core/config.h"
#include "core/logger.h"
#include <iostream>
#include <string>
#include <cstring>
#include <signal.h>
#include <unistd.h>

void print_usage(const char* program_name) {
    std::cout << "NERD: Network-Flow Editor" << std::endl;
    std::cout << "Editing living data streams that exist as active network circulation patterns" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << program_name << " [OPTIONS] [FLOW_NAME]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i, --interface <interface>  Network interface to use (default: eth0)" << std::endl;
    std::cout << "  -h, --help                   Show this help message" << std::endl;
    std::cout << "  -v, --version                Show version information" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << "                    # Start interactive mode" << std::endl;
    std::cout << "  " << program_name << " myflow            # Open flow 'myflow'" << std::endl;
    std::cout << "  " << program_name << " -i lo0 myflow     # Use loopback interface" << std::endl;
    std::cout << std::endl;
    std::cout << "Flow Commands:" << std::endl;
    std::cout << "  open <flow_name>             Open a flow" << std::endl;
    std::cout << "  append <text>                Append text to current flow" << std::endl;
    std::cout << "  insert <line> <text>         Insert text at a specific line" << std::endl;
    std::cout << "  delete <start> <end>         Delete lines from flow" << std::endl;
    std::cout << "  substitute <old> <new>       Replace text in flow" << std::endl;
    std::cout << "  print all                    Print all lines" << std::endl;
    std::cout << "  discover                     Discover existing flows" << std::endl;
    std::cout << "  list                         List active flows" << std::endl;
    std::cout << "  status                       Show current flow status" << std::endl;
    std::cout << "  write                        Write changes to circulation" << std::endl;
    std::cout << "  quit                         Exit editor" << std::endl;
}

void print_version() {
    std::cout << "NERD: Network-Flow Editor v1.0.0" << std::endl;
    std::cout << "Revolutionary file editing for living network processes" << std::endl;
    std::cout << "Production Ready" << std::endl;
}

// Signal handler for graceful shutdown
static nerd::FlowEditor* g_editor = nullptr;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        LOG_INFO("Received signal " + std::to_string(sig) + ", shutting down gracefully");
        if (g_editor) {
            g_editor->quit_editor();
        }
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    // Initialize configuration
    nerd::Config& config = nerd::Config::instance();
    std::string config_file = "/etc/nerd/nerd.conf";
    std::string interface;
    std::string flow_name;
    bool debug = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        
        if (arg == "-v" || arg == "--version") {
            print_version();
            return 0;
        }
        
        if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: Missing config file after " << arg << std::endl;
                return 1;
            }
        }
        else if (arg == "-d" || arg == "--debug") {
            debug = true;
        }
        else if (arg == "-i" || arg == "--interface") {
            if (i + 1 < argc) {
                interface = argv[++i];
            } else {
                std::cerr << "Error: Missing interface name after " << arg << std::endl;
                return 1;
            }
        }
        else if (arg[0] != '-') {
            // Non-option argument is the flow name
            if (flow_name.empty()) {
                flow_name = arg;
            } else {
                std::cerr << "Error: Multiple flow names specified" << std::endl;
                return 1;
            }
        }
        else {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Load configuration
    if (!config.load(config_file)) {
        std::cerr << "Failed to load configuration" << std::endl;
        return 1;
    }
    
    // Initialize logging
    nerd::LogLevel log_level = debug ? nerd::LogLevel::DEBUG : config.logging().log_level;
    nerd::Logger::instance().initialize(
        config.logging().log_file,
        log_level,
        config.logging().log_to_console,
        config.logging().log_to_file
    );
    
    LOG_INFO("NERD v1.0.0 starting up");
    LOG_INFO("Configuration loaded from: " + config_file);
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Use interface from command line or config
    if (interface.empty()) {
        interface = config.network().interface;
    }
    
    try {
        // Create the flow editor
        nerd::FlowEditor editor;
        g_editor = &editor;
        
        // Initialize network interface
        LOG_INFO("Initializing network interface: " + interface);
        if (!editor.initialize_network(interface)) {
            if (geteuid() != 0) {
                LOG_ERROR("Failed to initialize network interface. Root privileges required for raw socket access.");
                std::cerr << "Error: This program requires root privileges for network operations." << std::endl;
                std::cerr << "Please run with sudo: sudo " << argv[0] << std::endl;
                return 1;
            } else {
                LOG_WARNING("Failed to initialize network interface. Running in simulation mode.");
                std::cerr << "Warning: Failed to initialize network interface. Running in simulation mode." << std::endl;
            }
        } else {
            LOG_INFO("Network interface initialized successfully");
        }
        
        // Open flow if specified
        if (!flow_name.empty()) {
            LOG_INFO("Opening flow: " + flow_name);
            if (!editor.open_flow(flow_name)) {
                LOG_ERROR("Failed to open flow: " + flow_name);
                std::cerr << "Error: Failed to open flow '" << flow_name << "'" << std::endl;
                return 1;
            }
            LOG_INFO("Flow opened successfully: " + flow_name);
        }
        
        // Start interactive mode
        LOG_INFO("Starting interactive mode");
        editor.run_interactive();
        
        LOG_INFO("NERD shutting down gracefully");
        g_editor = nullptr;
        
    } catch (const std::exception& e) {
        LOG_CRITICAL("Fatal error: " + std::string(e.what()));
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        LOG_CRITICAL("Unknown fatal exception occurred");
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return 1;
    }
    
    LOG_INFO("NERD shutdown complete");
    return 0;
}