#include "editor/flow_editor.h"
#include <iostream>
#include <string>
#include <cstring>

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
    std::cout << "NERD: Network-Flow Editor v0.1.0" << std::endl;
    std::cout << "Revolutionary file editing for living network processes" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string interface = "eth0";
    std::string flow_name;
    
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
        
        if (arg == "-i" || arg == "--interface") {
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
    
    try {
        // Create the flow editor
        nerd::FlowEditor editor;
        
        // Initialize network interface
        std::cout << "Initializing network interface: " << interface << std::endl;
        if (!editor.initialize_network(interface)) {
            std::cerr << "Warning: Failed to initialize network interface. Running in simulation mode." << std::endl;
            std::cerr << "Note: Raw socket access requires root privileges." << std::endl;
        }
        
        // Open flow if specified
        if (!flow_name.empty()) {
            std::cout << "Opening flow: " << flow_name << std::endl;
            if (!editor.open_flow(flow_name)) {
                std::cerr << "Error: Failed to open flow '" << flow_name << "'" << std::endl;
                return 1;
            }
        }
        
        // Start interactive mode
        editor.run_interactive();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return 1;
    }
    
    return 0;
}