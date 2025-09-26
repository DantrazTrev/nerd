#include "testing/test_framework.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "NERD Test Runner" << std::endl;
    std::cout << "================" << std::endl;
    
    nerd::TestFramework framework;
    
    // Parse command line arguments
    bool verbose = false;
    std::string output_file;
    std::string suite_name;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "Error: Missing output filename" << std::endl;
                return 1;
            }
        } else if (arg == "-s" || arg == "--suite") {
            if (i + 1 < argc) {
                suite_name = argv[++i];
            } else {
                std::cerr << "Error: Missing suite name" << std::endl;
                return 1;
            }
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -v, --verbose     Enable verbose output" << std::endl;
            std::cout << "  -o, --output FILE Save results to file" << std::endl;
            std::cout << "  -s, --suite NAME  Run specific test suite" << std::endl;
            std::cout << "  -h, --help        Show this help message" << std::endl;
            return 0;
        }
    }
    
    // Configure framework
    framework.set_verbose_mode(verbose);
    if (!output_file.empty()) {
        framework.set_output_file(output_file);
    }
    
    // Run tests
    try {
        if (!suite_name.empty()) {
            std::cout << "Running test suite: " << suite_name << std::endl;
            framework.run_test_suite(suite_name);
        } else {
            std::cout << "Running all test suites..." << std::endl;
            framework.run_all_tests();
        }
        
        // Print results
        framework.print_results();
        
        // Return appropriate exit code
        if (framework.get_failed_tests() > 0 || framework.get_error_tests() > 0) {
            return 1;
        } else {
            return 0;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Test runner error: " << e.what() << std::endl;
        return 1;
    }
}