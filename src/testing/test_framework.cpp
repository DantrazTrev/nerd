#include "testing/test_framework.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

namespace nerd {

TestFramework::TestFramework() 
    : verbose_mode_(false), total_tests_(0), passed_tests_(0), 
      failed_tests_(0), skipped_tests_(0), error_tests_(0) {
    create_flow_tests();
    create_network_tests();
    create_workflow_tests();
    create_persistence_tests();
    create_integration_tests();
    create_performance_tests();
}

TestFramework::~TestFramework() = default;

void TestFramework::add_test_suite(const TestSuite& suite) {
    test_suites_[suite.name] = suite;
}

void TestFramework::remove_test_suite(const std::string& name) {
    test_suites_.erase(name);
}

TestSuite* TestFramework::get_test_suite(const std::string& name) {
    auto it = test_suites_.find(name);
    return (it != test_suites_.end()) ? &it->second : nullptr;
}

void TestFramework::run_all_tests() {
    std::cout << "Running all test suites..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    total_tests_ = 0;
    passed_tests_ = 0;
    failed_tests_ = 0;
    skipped_tests_ = 0;
    error_tests_ = 0;
    test_results_.clear();
    
    for (auto& pair : test_suites_) {
        if (pair.second.enabled) {
            run_test_suite(pair.first);
        }
    }
    
    print_summary();
    if (!output_file_.empty()) {
        save_results();
    }
}

void TestFramework::run_test_suite(const std::string& suite_name) {
    auto it = test_suites_.find(suite_name);
    if (it == test_suites_.end()) {
        std::cerr << "Test suite not found: " << suite_name << std::endl;
        return;
    }
    
    TestSuite& suite = it->second;
    std::cout << "\nRunning test suite: " << suite.name << std::endl;
    std::cout << "Description: " << suite.description << std::endl;
    std::cout << "Tests: " << suite.test_names.size() << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (const auto& test_name : suite.test_names) {
        run_test(suite_name, test_name);
    }
}

void TestFramework::run_test(const std::string& suite_name, const std::string& test_name) {
    TestResult result = execute_test(suite_name, test_name);
    test_results_.push_back(result);
    update_statistics(result);
    
    if (verbose_mode_) {
        print_test_result(result);
    }
}

void TestFramework::register_test(const std::string& suite_name, const std::string& test_name,
                                 std::function<TestResult()> test_func) {
    TestSuite* suite = get_test_suite(suite_name);
    if (!suite) {
        // Create new test suite
        TestSuite new_suite(suite_name, "Test suite for " + suite_name);
        add_test_suite(new_suite);
        suite = get_test_suite(suite_name);
    }
    
    suite->tests[test_name] = test_func;
    suite->test_names.push_back(test_name);
}

void TestFramework::register_test(const std::string& suite_name, const std::string& test_name,
                                 std::function<bool()> test_func) {
    register_test(suite_name, test_name, [test_name, test_func]() {
        TestResult result(test_name);
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            bool success = test_func();
            result.status = success ? TestStatus::PASSED : TestStatus::FAILED;
            result.message = success ? "Test passed" : "Test failed";
        } catch (const std::exception& e) {
            result.status = TestStatus::ERROR;
            result.message = "Exception: " + std::string(e.what());
            result.error_details = e.what();
        } catch (...) {
            result.status = TestStatus::ERROR;
            result.message = "Unknown exception occurred";
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        return result;
    });
}

void TestFramework::print_results() {
    print_summary();
    print_detailed_results();
}

void TestFramework::save_results() {
    save_results_to_file();
}

void TestFramework::create_flow_tests() {
    TestSuite flow_suite("flow_tests", "Flow file operations and management");
    
    // Test flow creation
    flow_suite.tests["test_flow_creation"] = []() {
        TestResult result("test_flow_creation");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            FlowFile flow(1, "test_flow");
            result.status = TestStatus::PASSED;
            result.message = "Flow created successfully";
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Failed to create flow: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test flow content operations
    flow_suite.tests["test_flow_content_operations"] = []() {
        TestResult result("test_flow_content_operations");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            FlowFile flow(1, "test_flow");
            flow.append_content("Hello, World!");
            flow.append_content("This is a test");
            
            std::string content = flow.content();
            if (content.find("Hello, World!") != std::string::npos &&
                content.find("This is a test") != std::string::npos) {
                result.status = TestStatus::PASSED;
                result.message = "Content operations work correctly";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Content not found in flow";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Exception in content operations: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test flow serialization
    flow_suite.tests["test_flow_serialization"] = []() {
        TestResult result("test_flow_serialization");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            FlowFile flow(1, "test_flow");
            flow.append_content("Serialization test");
            
            std::vector<uint8_t> serialized = flow.serialize_content();
            if (!serialized.empty()) {
                result.status = TestStatus::PASSED;
                result.message = "Serialization successful";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Serialization returned empty data";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Serialization failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    flow_suite.test_names = {"test_flow_creation", "test_flow_content_operations", "test_flow_serialization"};
    add_test_suite(flow_suite);
}

void TestFramework::create_network_tests() {
    TestSuite network_suite("network_tests", "Network flow operations and packet handling");
    
    // Test packet creation
    network_suite.tests["test_packet_creation"] = []() {
        TestResult result("test_packet_creation");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
            RawPacket packet(1, FLOW_DATA, data);
            
            if (packet.is_valid() && packet.header().flow_id == 1) {
                result.status = TestStatus::PASSED;
                result.message = "Packet created successfully";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Packet validation failed";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Packet creation failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test packet serialization
    network_suite.tests["test_packet_serialization"] = []() {
        TestResult result("test_packet_serialization");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            std::vector<uint8_t> data = {'T', 'e', 's', 't'};
            RawPacket packet(1, FLOW_DATA, data);
            
            std::vector<uint8_t> serialized = packet.serialize();
            if (!serialized.empty()) {
                RawPacket deserialized;
                if (deserialized.deserialize(serialized) && deserialized.is_valid()) {
                    result.status = TestStatus::PASSED;
                    result.message = "Packet serialization/deserialization successful";
                } else {
                    result.status = TestStatus::FAILED;
                    result.message = "Packet deserialization failed";
                }
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Packet serialization returned empty data";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Packet serialization failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test packet stream
    network_suite.tests["test_packet_stream"] = []() {
        TestResult result("test_packet_stream");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            PacketStream stream(1);
            
            std::vector<uint8_t> data1 = {'A', 'B', 'C'};
            std::vector<uint8_t> data2 = {'D', 'E', 'F'};
            
            RawPacket packet1(1, FLOW_DATA, data1);
            RawPacket packet2(1, FLOW_DATA, data2);
            
            stream.add_packet(packet1);
            stream.add_packet(packet2);
            
            if (stream.packets().size() == 2) {
                result.status = TestStatus::PASSED;
                result.message = "Packet stream operations successful";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Expected 2 packets, got " + std::to_string(stream.packets().size());
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Packet stream test failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    network_suite.test_names = {"test_packet_creation", "test_packet_serialization", "test_packet_stream"};
    add_test_suite(network_suite);
}

void TestFramework::create_workflow_tests() {
    TestSuite workflow_suite("workflow_tests", "Workflow management and automation");
    
    // Test workflow manager creation
    workflow_suite.tests["test_workflow_manager_creation"] = []() {
        TestResult result("test_workflow_manager_creation");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            WorkflowManager manager;
            result.status = TestStatus::PASSED;
            result.message = "Workflow manager created successfully";
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Workflow manager creation failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test workflow creation
    workflow_suite.tests["test_workflow_creation"] = []() {
        TestResult result("test_workflow_creation");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            WorkflowManager manager;
            manager.create_default_workflows();
            
            auto workflows = manager.list_workflows();
            if (!workflows.empty()) {
                result.status = TestStatus::PASSED;
                result.message = "Default workflows created successfully";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "No workflows created";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Workflow creation failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test cost tracking
    workflow_suite.tests["test_cost_tracking"] = []() {
        TestResult result("test_cost_tracking");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            WorkflowManager manager;
            manager.track_flow_cost(1, "test_flow");
            manager.update_flow_cost(1, 10.0, 5.0, 2.0, 50.0, 100, 1024);
            
            FlowCost* cost = manager.get_flow_cost(1);
            if (cost && cost->bandwidth_cost == 10.0) {
                result.status = TestStatus::PASSED;
                result.message = "Cost tracking works correctly";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Cost tracking failed";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Cost tracking failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    workflow_suite.test_names = {"test_workflow_manager_creation", "test_workflow_creation", "test_cost_tracking"};
    add_test_suite(workflow_suite);
}

void TestFramework::create_persistence_tests() {
    TestSuite persistence_suite("persistence_tests", "Flow persistence and recovery");
    
    // Test persistence creation
    persistence_suite.tests["test_persistence_creation"] = []() {
        TestResult result("test_persistence_creation");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            FlowPersistence persistence("/tmp/nerd_test_persistence");
            result.status = TestStatus::PASSED;
            result.message = "Persistence system created successfully";
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Persistence creation failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test flow saving
    persistence_suite.tests["test_flow_saving"] = []() {
        TestResult result("test_flow_saving");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            FlowPersistence persistence("/tmp/nerd_test_persistence");
            FlowFile flow(1, "test_flow");
            flow.append_content("Test content for persistence");
            
            if (persistence.save_flow(flow)) {
                result.status = TestStatus::PASSED;
                result.message = "Flow saved successfully";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Flow saving failed";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Flow saving failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test checkpoint creation
    persistence_suite.tests["test_checkpoint_creation"] = []() {
        TestResult result("test_checkpoint_creation");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            FlowPersistence persistence("/tmp/nerd_test_persistence");
            
            if (persistence.create_checkpoint()) {
                result.status = TestStatus::PASSED;
                result.message = "Checkpoint created successfully";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Checkpoint creation failed";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Checkpoint creation failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    persistence_suite.test_names = {"test_persistence_creation", "test_flow_saving", "test_checkpoint_creation"};
    add_test_suite(persistence_suite);
}

void TestFramework::create_integration_tests() {
    TestSuite integration_suite("integration_tests", "End-to-end integration testing");
    
    // Test complete flow lifecycle
    integration_suite.tests["test_complete_flow_lifecycle"] = []() {
        TestResult result("test_complete_flow_lifecycle");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Create flow
            FlowFile flow(1, "integration_test_flow");
            flow.append_content("Integration test content");
            
            // Create persistence
            FlowPersistence persistence("/tmp/nerd_integration_test");
            
            // Save flow
            if (!persistence.save_flow(flow)) {
                result.status = TestStatus::FAILED;
                result.message = "Failed to save flow";
                return result;
            }
            
            // Create new flow and load
            FlowFile loaded_flow(1, "loaded_flow");
            if (!persistence.load_flow(1, loaded_flow)) {
                result.status = TestStatus::FAILED;
                result.message = "Failed to load flow";
                return result;
            }
            
            // Verify content
            if (loaded_flow.content().find("Integration test content") != std::string::npos) {
                result.status = TestStatus::PASSED;
                result.message = "Complete flow lifecycle test passed";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Content mismatch after load";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Integration test failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    integration_suite.test_names = {"test_complete_flow_lifecycle"};
    add_test_suite(integration_suite);
}

void TestFramework::create_performance_tests() {
    TestSuite performance_suite("performance_tests", "Performance and stress testing");
    
    // Test large flow handling
    performance_suite.tests["test_large_flow_handling"] = []() {
        TestResult result("test_large_flow_handling");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            FlowFile flow(1, "large_flow");
            
            // Add large amount of content
            for (int i = 0; i < 1000; ++i) {
                flow.append_content("Line " + std::to_string(i) + " of large flow content");
            }
            
            if (flow.content().length() > 10000) {
                result.status = TestStatus::PASSED;
                result.message = "Large flow handling successful";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Large flow content too small";
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Large flow test failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    // Test packet throughput
    performance_suite.tests["test_packet_throughput"] = []() {
        TestResult result("test_packet_throughput");
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            PacketStream stream(1);
            std::vector<uint8_t> data = {'T', 'e', 's', 't'};
            
            // Create many packets
            for (int i = 0; i < 1000; ++i) {
                RawPacket packet(1, FLOW_DATA, data);
                packet.set_sequence(i);
                stream.add_packet(packet);
            }
            
            if (stream.packets().size() == 1000) {
                result.status = TestStatus::PASSED;
                result.message = "Packet throughput test successful";
            } else {
                result.status = TestStatus::FAILED;
                result.message = "Expected 1000 packets, got " + std::to_string(stream.packets().size());
            }
        } catch (const std::exception& e) {
            result.status = TestStatus::FAILED;
            result.message = "Packet throughput test failed: " + std::string(e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    };
    
    performance_suite.test_names = {"test_large_flow_handling", "test_packet_throughput"};
    add_test_suite(performance_suite);
}

TestResult TestFramework::execute_test(const std::string& suite_name, const std::string& test_name) {
    TestSuite* suite = get_test_suite(suite_name);
    if (!suite) {
        TestResult result(test_name);
        result.status = TestStatus::ERROR;
        result.message = "Test suite not found: " + suite_name;
        return result;
    }
    
    auto it = suite->tests.find(test_name);
    if (it == suite->tests.end()) {
        TestResult result(test_name);
        result.status = TestStatus::ERROR;
        result.message = "Test not found: " + test_name;
        return result;
    }
    
    return it->second();
}

void TestFramework::update_statistics(const TestResult& result) {
    total_tests_++;
    
    switch (result.status) {
        case TestStatus::PASSED:
            passed_tests_++;
            break;
        case TestStatus::FAILED:
            failed_tests_++;
            break;
        case TestStatus::SKIPPED:
            skipped_tests_++;
            break;
        case TestStatus::ERROR:
            error_tests_++;
            break;
    }
}

void TestFramework::print_test_result(const TestResult& result) {
    std::string status_str;
    switch (result.status) {
        case TestStatus::PASSED:
            status_str = "PASS";
            break;
        case TestStatus::FAILED:
            status_str = "FAIL";
            break;
        case TestStatus::SKIPPED:
            status_str = "SKIP";
            break;
        case TestStatus::ERROR:
            status_str = "ERROR";
            break;
    }
    
    std::cout << "  " << std::setw(8) << status_str << " " 
              << std::setw(30) << result.test_name << " "
              << std::setw(8) << result.duration.count() << "ms"
              << " - " << result.message << std::endl;
}

void TestFramework::print_summary() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total tests: " << total_tests_ << std::endl;
    std::cout << "Passed: " << passed_tests_ << std::endl;
    std::cout << "Failed: " << failed_tests_ << std::endl;
    std::cout << "Skipped: " << skipped_tests_ << std::endl;
    std::cout << "Errors: " << error_tests_ << std::endl;
    
    if (total_tests_ > 0) {
        double pass_rate = (double)passed_tests_ / total_tests_ * 100.0;
        std::cout << "Pass rate: " << std::fixed << std::setprecision(1) << pass_rate << "%" << std::endl;
    }
}

void TestFramework::print_detailed_results() {
    std::cout << "\nDetailed Results:" << std::endl;
    std::cout << "========================================" << std::endl;
    
    for (const auto& result : test_results_) {
        print_test_result(result);
        if (!result.error_details.empty()) {
            std::cout << "    Error details: " << result.error_details << std::endl;
        }
    }
}

void TestFramework::save_results_to_file() {
    std::ofstream file(output_file_);
    if (!file.is_open()) {
        std::cerr << "Failed to open output file: " << output_file_ << std::endl;
        return;
    }
    
    file << "NERD Test Results" << std::endl;
    file << "=================" << std::endl;
    file << "Total tests: " << total_tests_ << std::endl;
    file << "Passed: " << passed_tests_ << std::endl;
    file << "Failed: " << failed_tests_ << std::endl;
    file << "Skipped: " << skipped_tests_ << std::endl;
    file << "Errors: " << error_tests_ << std::endl;
    file << std::endl;
    
    for (const auto& result : test_results_) {
        file << result.test_name << ": " << result.message << std::endl;
        if (!result.error_details.empty()) {
            file << "  Error: " << result.error_details << std::endl;
        }
    }
    
    file.close();
    std::cout << "Results saved to: " << output_file_ << std::endl;
}

// TestUtils implementation
std::unique_ptr<FlowFile> TestUtils::create_test_flow(const std::string& name, const std::string& content) {
    auto flow = std::make_unique<FlowFile>(1, name);
    flow->write_to_flow(content);
    return flow;
}

std::unique_ptr<NetworkFlow> TestUtils::create_test_network_flow() {
    return std::make_unique<MockNetworkFlow>();
}

std::unique_ptr<WorkflowManager> TestUtils::create_test_workflow_manager() {
    auto manager = std::make_unique<WorkflowManager>();
    manager->create_default_workflows();
    return manager;
}

std::unique_ptr<FlowPersistence> TestUtils::create_test_persistence() {
    return std::make_unique<MockFlowPersistence>();
}

std::string TestUtils::generate_test_content(size_t size) {
    std::string content;
    content.reserve(size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(32, 126); // Printable ASCII
    
    for (size_t i = 0; i < size; ++i) {
        content += static_cast<char>(dis(gen));
    }
    
    return content;
}

std::vector<uint8_t> TestUtils::generate_test_packet_data(size_t size) {
    std::vector<uint8_t> data;
    data.reserve(size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (size_t i = 0; i < size; ++i) {
        data.push_back(static_cast<uint8_t>(dis(gen)));
    }
    
    return data;
}

CirculationPattern TestUtils::create_test_circulation_pattern() {
    CirculationPattern pattern;
    pattern.id = 1;
    pattern.name = "test_pattern";
    pattern.circulation_rate = 10;
    pattern.max_packet_age = 30000000;
    pattern.auto_sustain = true;
    return pattern;
}

bool TestUtils::simulate_network_operation() {
    // Simulate network operation
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return true;
}

bool TestUtils::simulate_network_disruption() {
    // Simulate network disruption
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return false;
}

bool TestUtils::simulate_high_latency() {
    // Simulate high latency
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return true;
}

std::chrono::milliseconds TestUtils::measure_execution_time(std::function<void()> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}

size_t TestUtils::measure_memory_usage() {
    // Simplified memory usage measurement
    return sizeof(FlowFile) + sizeof(NetworkFlow) + sizeof(WorkflowManager);
}

bool TestUtils::create_test_directory(const std::string& path) {
    // Simplified implementation - just return true for now
    return true;
}

bool TestUtils::cleanup_test_directory(const std::string& path) {
    // Simplified implementation - just return true for now
    return true;
}

bool TestUtils::file_exists(const std::string& path) {
    // Simplified implementation - just return true for now
    return true;
}

size_t TestUtils::get_file_size(const std::string& path) {
    // Simplified implementation - return 0 for now
    return 0;
}

} // namespace nerd