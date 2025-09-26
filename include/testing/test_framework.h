#pragma once

#include "core/flow_file.h"
#include "network/flow.h"
#include "workflow/workflow_manager.h"
#include "persistence/flow_persistence.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <memory>

namespace nerd {

// Test result status
enum class TestStatus {
    PASSED,
    FAILED,
    SKIPPED,
    ERROR
};

// Test result information
struct TestResult {
    std::string test_name;
    TestStatus status;
    std::string message;
    std::chrono::milliseconds duration;
    std::string error_details;
    
    TestResult() : status(TestStatus::SKIPPED), duration(0) {}
    TestResult(const std::string& name) : test_name(name), status(TestStatus::SKIPPED), duration(0) {}
};

// Test suite information
struct TestSuite {
    std::string name;
    std::string description;
    std::vector<std::string> test_names;
    std::map<std::string, std::function<TestResult()>> tests;
    bool enabled;
    
    TestSuite() : enabled(true) {}
    TestSuite(const std::string& suite_name, const std::string& desc)
        : name(suite_name), description(desc), enabled(true) {}
};

// Test framework for comprehensive testing
class TestFramework {
private:
    std::map<std::string, TestSuite> test_suites_;
    std::vector<TestResult> test_results_;
    bool verbose_mode_;
    std::string output_file_;
    
    // Test statistics
    size_t total_tests_;
    size_t passed_tests_;
    size_t failed_tests_;
    size_t skipped_tests_;
    size_t error_tests_;
    
public:
    TestFramework();
    ~TestFramework();
    
    // Test suite management
    void add_test_suite(const TestSuite& suite);
    void remove_test_suite(const std::string& name);
    TestSuite* get_test_suite(const std::string& name);
    
    // Test execution
    void run_all_tests();
    void run_test_suite(const std::string& suite_name);
    void run_test(const std::string& suite_name, const std::string& test_name);
    
    // Test registration
    void register_test(const std::string& suite_name, const std::string& test_name,
                      std::function<TestResult()> test_func);
    void register_test(const std::string& suite_name, const std::string& test_name,
                      std::function<bool()> test_func);
    
    // Configuration
    void set_verbose_mode(bool enabled) { verbose_mode_ = enabled; }
    void set_output_file(const std::string& filename) { output_file_ = filename; }
    
    // Results
    void print_results();
    void save_results();
    std::vector<TestResult> get_results() const { return test_results_; }
    
    // Statistics
    size_t get_total_tests() const { return total_tests_; }
    size_t get_passed_tests() const { return passed_tests_; }
    size_t get_failed_tests() const { return failed_tests_; }
    size_t get_skipped_tests() const { return skipped_tests_; }
    size_t get_error_tests() const { return error_tests_; }
    
    // Built-in test suites
    void create_flow_tests();
    void create_network_tests();
    void create_workflow_tests();
    void create_persistence_tests();
    void create_integration_tests();
    void create_performance_tests();
    
private:
    // Test execution helpers
    TestResult execute_test(const std::string& suite_name, const std::string& test_name);
    void update_statistics(const TestResult& result);
    void print_test_result(const TestResult& result);
    
    // Output helpers
    void print_summary();
    void print_detailed_results();
    void save_results_to_file();
    
    // Test utilities
    bool assert_true(bool condition, const std::string& message = "");
    bool assert_false(bool condition, const std::string& message = "");
    bool assert_equals(const std::string& expected, const std::string& actual, const std::string& message = "");
    bool assert_not_null(void* ptr, const std::string& message = "");
    bool assert_throws(std::function<void()> func, const std::string& message = "");
};

// Test utilities and helpers
class TestUtils {
public:
    // Flow creation helpers
    static std::unique_ptr<FlowFile> create_test_flow(const std::string& name, const std::string& content);
    static std::unique_ptr<NetworkFlow> create_test_network_flow();
    static std::unique_ptr<WorkflowManager> create_test_workflow_manager();
    static std::unique_ptr<FlowPersistence> create_test_persistence();
    
    // Test data generators
    static std::string generate_test_content(size_t size);
    static std::vector<uint8_t> generate_test_packet_data(size_t size);
    static CirculationPattern create_test_circulation_pattern();
    
    // Network simulation helpers
    static bool simulate_network_operation();
    static bool simulate_network_disruption();
    static bool simulate_high_latency();
    
    // Performance measurement
    static std::chrono::milliseconds measure_execution_time(std::function<void()> func);
    static size_t measure_memory_usage();
    
    // File system helpers
    static bool create_test_directory(const std::string& path);
    static bool cleanup_test_directory(const std::string& path);
    static bool file_exists(const std::string& path);
    static size_t get_file_size(const std::string& path);
};

// Mock objects for testing
class MockNetworkFlow : public NetworkFlow {
public:
    MockNetworkFlow() : NetworkFlow() {}
    
    // Override methods for testing
    bool initialize_interface(const std::string& interface) {
        return true; // Always succeed in mock
    }
    
    void inject_packet(const RawPacket& packet) {
        // Mock implementation - just log
        // Note: This would need proper logging in real implementation
    }
};

class MockFlowPersistence : public FlowPersistence {
public:
    MockFlowPersistence() : FlowPersistence("/tmp/nerd_test_persistence") {}
    
    // Override methods for testing
    bool save_flow(const FlowFile& flow) {
        // Mock implementation - always succeed
        return true;
    }
    
    bool load_flow(FlowID flow_id, FlowFile& flow) {
        // Mock implementation - create dummy flow
        flow.write_to_flow("Mock loaded content");
        return true;
    }
};

} // namespace nerd