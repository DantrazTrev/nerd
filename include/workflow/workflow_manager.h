#pragma once

#include "core/flow_file.h"
#include "network/flow.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>

namespace nerd {

// Workflow trigger conditions
enum class TriggerType {
    TIME_BASED,         // Triggered by time intervals
    FLOW_MODIFIED,      // Triggered when flow content changes
    COST_THRESHOLD,     // Triggered when cost exceeds threshold
    NETWORK_CONDITION,  // Triggered by network state changes
    USER_COMMAND        // Triggered by user action
};

// Workflow action types
enum class ActionType {
    OPTIMIZE_CIRCULATION,   // Optimize flow circulation patterns
    COMPRESS_FLOW,          // Compress flow data
    MIGRATE_FLOW,           // Migrate flow to different nodes
    BACKUP_FLOW,            // Create flow backup
    CLEANUP_OLD_PACKETS,    // Clean up old packets
    ADJUST_RATE,            // Adjust circulation rate
    NOTIFY_USER,            // Send notification to user
    AUTO_SAVE,              // Auto-save flow changes
    COST_ANALYSIS           // Perform cost analysis
};

// Workflow trigger definition
struct WorkflowTrigger {
    TriggerType type;
    std::string condition;      // Condition expression
    std::chrono::seconds interval; // For time-based triggers
    double threshold;           // For threshold-based triggers
    bool enabled;
    
    WorkflowTrigger() : type(TriggerType::TIME_BASED), interval(300), threshold(0.0), enabled(true) {}
};

// Workflow action definition
struct WorkflowAction {
    ActionType type;
    std::string parameters;     // Action-specific parameters
    int priority;               // Action priority (higher = more important)
    bool enabled;
    
    WorkflowAction() : type(ActionType::OPTIMIZE_CIRCULATION), priority(1), enabled(true) {}
};

// Complete workflow definition
struct Workflow {
    std::string name;
    std::string description;
    WorkflowTrigger trigger;
    std::vector<WorkflowAction> actions;
    bool enabled;
    std::chrono::time_point<std::chrono::system_clock> last_run;
    int run_count;
    
    Workflow() : enabled(true), run_count(0) {}
};

// Cost tracking for flows
struct FlowCost {
    FlowID flow_id;
    std::string flow_name;
    double bandwidth_cost;      // Cost per MB
    double cpu_cost;           // Cost per CPU second
    double memory_cost;        // Cost per MB of memory
    double network_latency;    // Average latency in ms
    uint64_t packets_sent;     // Total packets sent
    uint64_t bytes_transferred; // Total bytes transferred
    std::chrono::time_point<std::chrono::system_clock> last_updated;
    
    FlowCost() : flow_id(0), bandwidth_cost(0.0), cpu_cost(0.0), memory_cost(0.0), 
                 network_latency(0.0), packets_sent(0), bytes_transferred(0) {}
};

// Workflow execution context
struct WorkflowContext {
    FlowFile* flow;
    FlowCost* cost_info;
    std::map<std::string, std::string> variables;
    std::chrono::time_point<std::chrono::system_clock> start_time;
    
    WorkflowContext() : flow(nullptr), cost_info(nullptr) {}
};

// Workflow manager - manages automated workflows and cost optimization
class WorkflowManager {
private:
    std::map<std::string, Workflow> workflows_;
    std::map<FlowID, FlowCost> flow_costs_;
    std::thread workflow_thread_;
    std::atomic<bool> running_;
    mutable std::mutex workflows_mutex_;
    mutable std::mutex costs_mutex_;
    
    // Cost tracking
    double total_bandwidth_cost_;
    double total_cpu_cost_;
    double total_memory_cost_;
    std::chrono::time_point<std::chrono::system_clock> last_cost_reset_;
    
public:
    WorkflowManager();
    ~WorkflowManager();
    
    // Workflow management
    void add_workflow(const Workflow& workflow);
    void remove_workflow(const std::string& name);
    void enable_workflow(const std::string& name, bool enabled = true);
    Workflow* get_workflow(const std::string& name);
    std::vector<std::string> list_workflows() const;
    
    // Workflow execution
    void execute_workflow(const std::string& name, WorkflowContext& context);
    void execute_all_workflows();
    void trigger_workflow(const std::string& name, WorkflowContext& context);
    
    // Cost tracking
    void track_flow_cost(FlowID flow_id, const std::string& flow_name);
    void update_flow_cost(FlowID flow_id, double bandwidth, double cpu, double memory, 
                         double latency, uint64_t packets, uint64_t bytes);
    FlowCost* get_flow_cost(FlowID flow_id);
    double get_total_cost() const;
    void reset_cost_tracking();
    
    // Cost optimization workflows
    void create_cost_optimization_workflows();
    void create_performance_workflows();
    void create_maintenance_workflows();
    
    // Workflow control
    void start_workflow_engine();
    void stop_workflow_engine();
    bool is_running() const { return running_; }
    
    // Built-in workflows
    void create_default_workflows();
    
private:
    void workflow_worker();
    bool evaluate_trigger(const WorkflowTrigger& trigger, const WorkflowContext& context);
    void execute_action(const WorkflowAction& action, WorkflowContext& context);
    void optimize_circulation_pattern(FlowFile* flow);
    void compress_flow_data(FlowFile* flow);
    void migrate_flow_to_optimal_node(FlowFile* flow);
    void backup_flow(FlowFile* flow);
    void cleanup_old_packets(FlowFile* flow);
    void adjust_circulation_rate(FlowFile* flow, double factor);
    void notify_user(const std::string& message);
    void perform_cost_analysis();
    
    // Cost calculation helpers
    double calculate_bandwidth_cost(uint64_t bytes, double rate_per_mb);
    double calculate_cpu_cost(double cpu_seconds, double rate_per_second);
    double calculate_memory_cost(uint64_t memory_mb, double rate_per_mb);
};

} // namespace nerd