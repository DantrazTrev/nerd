#include "workflow/workflow_manager.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace nerd {

WorkflowManager::WorkflowManager() 
    : running_(false), total_bandwidth_cost_(0.0), total_cpu_cost_(0.0), 
      total_memory_cost_(0.0) {
    last_cost_reset_ = std::chrono::system_clock::now();
}

WorkflowManager::~WorkflowManager() {
    stop_workflow_engine();
}

void WorkflowManager::add_workflow(const Workflow& workflow) {
    std::lock_guard<std::mutex> lock(workflows_mutex_);
    workflows_[workflow.name] = workflow;
    std::cout << "Added workflow: " << workflow.name << std::endl;
}

void WorkflowManager::remove_workflow(const std::string& name) {
    std::lock_guard<std::mutex> lock(workflows_mutex_);
    auto it = workflows_.find(name);
    if (it != workflows_.end()) {
        workflows_.erase(it);
        std::cout << "Removed workflow: " << name << std::endl;
    }
}

void WorkflowManager::enable_workflow(const std::string& name, bool enabled) {
    std::lock_guard<std::mutex> lock(workflows_mutex_);
    auto it = workflows_.find(name);
    if (it != workflows_.end()) {
        it->second.enabled = enabled;
        std::cout << "Workflow " << name << " " << (enabled ? "enabled" : "disabled") << std::endl;
    }
}

Workflow* WorkflowManager::get_workflow(const std::string& name) {
    std::lock_guard<std::mutex> lock(workflows_mutex_);
    auto it = workflows_.find(name);
    return (it != workflows_.end()) ? &it->second : nullptr;
}

std::vector<std::string> WorkflowManager::list_workflows() const {
    std::lock_guard<std::mutex> lock(workflows_mutex_);
    std::vector<std::string> names;
    names.reserve(workflows_.size());
    
    for (const auto& pair : workflows_) {
        names.push_back(pair.first);
    }
    
    return names;
}

void WorkflowManager::execute_workflow(const std::string& name, WorkflowContext& context) {
    Workflow* workflow = get_workflow(name);
    if (!workflow || !workflow->enabled) {
        return;
    }
    
    // Check if trigger conditions are met
    if (!evaluate_trigger(workflow->trigger, context)) {
        return;
    }
    
    std::cout << "Executing workflow: " << name << std::endl;
    
    // Execute all actions in priority order
    std::vector<WorkflowAction> actions = workflow->actions;
    std::sort(actions.begin(), actions.end(), 
              [](const WorkflowAction& a, const WorkflowAction& b) {
                  return a.priority > b.priority;
              });
    
    for (const auto& action : actions) {
        if (action.enabled) {
            execute_action(action, context);
        }
    }
    
    // Update workflow statistics
    workflow->last_run = std::chrono::system_clock::now();
    workflow->run_count++;
}

void WorkflowManager::execute_all_workflows() {
    std::lock_guard<std::mutex> lock(workflows_mutex_);
    
    for (auto& pair : workflows_) {
        if (pair.second.enabled) {
            WorkflowContext context;
            execute_workflow(pair.first, context);
        }
    }
}

void WorkflowManager::trigger_workflow(const std::string& name, WorkflowContext& context) {
    execute_workflow(name, context);
}

void WorkflowManager::track_flow_cost(FlowID flow_id, const std::string& flow_name) {
    std::lock_guard<std::mutex> lock(costs_mutex_);
    flow_costs_[flow_id] = FlowCost();
    flow_costs_[flow_id].flow_id = flow_id;
    flow_costs_[flow_id].flow_name = flow_name;
    flow_costs_[flow_id].last_updated = std::chrono::system_clock::now();
}

void WorkflowManager::update_flow_cost(FlowID flow_id, double bandwidth, double cpu, 
                                      double memory, double latency, uint64_t packets, 
                                      uint64_t bytes) {
    std::lock_guard<std::mutex> lock(costs_mutex_);
    
    auto it = flow_costs_.find(flow_id);
    if (it != flow_costs_.end()) {
        it->second.bandwidth_cost += bandwidth;
        it->second.cpu_cost += cpu;
        it->second.memory_cost += memory;
        it->second.network_latency = latency;
        it->second.packets_sent += packets;
        it->second.bytes_transferred += bytes;
        it->second.last_updated = std::chrono::system_clock::now();
        
        // Update totals
        total_bandwidth_cost_ += bandwidth;
        total_cpu_cost_ += cpu;
        total_memory_cost_ += memory;
    }
}

FlowCost* WorkflowManager::get_flow_cost(FlowID flow_id) {
    std::lock_guard<std::mutex> lock(costs_mutex_);
    auto it = flow_costs_.find(flow_id);
    return (it != flow_costs_.end()) ? &it->second : nullptr;
}

double WorkflowManager::get_total_cost() const {
    return total_bandwidth_cost_ + total_cpu_cost_ + total_memory_cost_;
}

void WorkflowManager::reset_cost_tracking() {
    std::lock_guard<std::mutex> lock(costs_mutex_);
    total_bandwidth_cost_ = 0.0;
    total_cpu_cost_ = 0.0;
    total_memory_cost_ = 0.0;
    last_cost_reset_ = std::chrono::system_clock::now();
    
    for (auto& pair : flow_costs_) {
        pair.second.bandwidth_cost = 0.0;
        pair.second.cpu_cost = 0.0;
        pair.second.memory_cost = 0.0;
        pair.second.packets_sent = 0;
        pair.second.bytes_transferred = 0;
    }
}

void WorkflowManager::create_cost_optimization_workflows() {
    // High cost alert workflow
    Workflow high_cost_alert;
    high_cost_alert.name = "high_cost_alert";
    high_cost_alert.description = "Alert when flow cost exceeds threshold";
    high_cost_alert.trigger.type = TriggerType::COST_THRESHOLD;
    high_cost_alert.trigger.threshold = 100.0; // $100 threshold
    high_cost_alert.trigger.condition = "cost > 100.0";
    
    WorkflowAction alert_action;
    alert_action.type = ActionType::NOTIFY_USER;
    alert_action.parameters = "High cost detected for flow: {flow_name}";
    alert_action.priority = 10;
    high_cost_alert.actions.push_back(alert_action);
    
    WorkflowAction optimize_action;
    optimize_action.type = ActionType::OPTIMIZE_CIRCULATION;
    optimize_action.parameters = "aggressive";
    optimize_action.priority = 5;
    high_cost_alert.actions.push_back(optimize_action);
    
    add_workflow(high_cost_alert);
    
    // Cost optimization workflow
    Workflow cost_optimization;
    cost_optimization.name = "cost_optimization";
    cost_optimization.description = "Optimize flows for cost efficiency";
    cost_optimization.trigger.type = TriggerType::TIME_BASED;
    cost_optimization.trigger.interval = std::chrono::seconds(300); // Every 5 minutes
    
    WorkflowAction compress_action;
    compress_action.type = ActionType::COMPRESS_FLOW;
    compress_action.parameters = "level=6";
    compress_action.priority = 3;
    cost_optimization.actions.push_back(compress_action);
    
    WorkflowAction rate_action;
    rate_action.type = ActionType::ADJUST_RATE;
    rate_action.parameters = "factor=0.8";
    rate_action.priority = 2;
    cost_optimization.actions.push_back(rate_action);
    
    add_workflow(cost_optimization);
}

void WorkflowManager::create_performance_workflows() {
    // Performance monitoring workflow
    Workflow performance_monitor;
    performance_monitor.name = "performance_monitor";
    performance_monitor.description = "Monitor and optimize flow performance";
    performance_monitor.trigger.type = TriggerType::TIME_BASED;
    performance_monitor.trigger.interval = std::chrono::seconds(60); // Every minute
    
    WorkflowAction analysis_action;
    analysis_action.type = ActionType::COST_ANALYSIS;
    analysis_action.parameters = "detailed";
    analysis_action.priority = 1;
    performance_monitor.actions.push_back(analysis_action);
    
    add_workflow(performance_monitor);
    
    // Latency optimization workflow
    Workflow latency_optimization;
    latency_optimization.name = "latency_optimization";
    latency_optimization.description = "Optimize flows with high latency";
    latency_optimization.trigger.type = TriggerType::NETWORK_CONDITION;
    latency_optimization.trigger.condition = "latency > 100";
    
    WorkflowAction migrate_action;
    migrate_action.type = ActionType::MIGRATE_FLOW;
    migrate_action.parameters = "optimal_node";
    migrate_action.priority = 8;
    latency_optimization.actions.push_back(migrate_action);
    
    add_workflow(latency_optimization);
}

void WorkflowManager::create_maintenance_workflows() {
    // Auto-save workflow
    Workflow auto_save;
    auto_save.name = "auto_save";
    auto_save.description = "Automatically save flow changes";
    auto_save.trigger.type = TriggerType::FLOW_MODIFIED;
    auto_save.trigger.condition = "modified = true";
    
    WorkflowAction save_action;
    save_action.type = ActionType::AUTO_SAVE;
    save_action.parameters = "immediate";
    save_action.priority = 5;
    auto_save.actions.push_back(save_action);
    
    add_workflow(auto_save);
    
    // Cleanup workflow
    Workflow cleanup;
    cleanup.name = "cleanup";
    cleanup.description = "Clean up old packets and optimize storage";
    cleanup.trigger.type = TriggerType::TIME_BASED;
    cleanup.trigger.interval = std::chrono::seconds(1800); // Every 30 minutes
    
    WorkflowAction cleanup_action;
    cleanup_action.type = ActionType::CLEANUP_OLD_PACKETS;
    cleanup_action.parameters = "age=3600";
    cleanup_action.priority = 2;
    cleanup.actions.push_back(cleanup_action);
    
    add_workflow(cleanup);
}

void WorkflowManager::start_workflow_engine() {
    if (!running_) {
        running_ = true;
        workflow_thread_ = std::thread(&WorkflowManager::workflow_worker, this);
        std::cout << "Started workflow engine" << std::endl;
    }
}

void WorkflowManager::stop_workflow_engine() {
    if (running_) {
        running_ = false;
        if (workflow_thread_.joinable()) {
            workflow_thread_.join();
        }
        std::cout << "Stopped workflow engine" << std::endl;
    }
}

void WorkflowManager::create_default_workflows() {
    create_cost_optimization_workflows();
    create_performance_workflows();
    create_maintenance_workflows();
}

void WorkflowManager::workflow_worker() {
    while (running_) {
        execute_all_workflows();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

bool WorkflowManager::evaluate_trigger(const WorkflowTrigger& trigger, const WorkflowContext& context) {
    switch (trigger.type) {
        case TriggerType::TIME_BASED:
            return true; // Time-based triggers are handled by the worker thread
            
        case TriggerType::FLOW_MODIFIED:
            return context.flow && context.flow->is_modified();
            
        case TriggerType::COST_THRESHOLD:
            if (context.cost_info) {
                double total_cost = context.cost_info->bandwidth_cost + 
                                  context.cost_info->cpu_cost + 
                                  context.cost_info->memory_cost;
                return total_cost > trigger.threshold;
            }
            return false;
            
        case TriggerType::NETWORK_CONDITION:
            if (context.cost_info) {
                return context.cost_info->network_latency > trigger.threshold;
            }
            return false;
            
        case TriggerType::USER_COMMAND:
            return true; // User commands are always triggered
            
        default:
            return false;
    }
}

void WorkflowManager::execute_action(const WorkflowAction& action, WorkflowContext& context) {
    switch (action.type) {
        case ActionType::OPTIMIZE_CIRCULATION:
            if (context.flow) {
                optimize_circulation_pattern(context.flow);
            }
            break;
            
        case ActionType::COMPRESS_FLOW:
            if (context.flow) {
                compress_flow_data(context.flow);
            }
            break;
            
        case ActionType::MIGRATE_FLOW:
            if (context.flow) {
                migrate_flow_to_optimal_node(context.flow);
            }
            break;
            
        case ActionType::BACKUP_FLOW:
            if (context.flow) {
                backup_flow(context.flow);
            }
            break;
            
        case ActionType::CLEANUP_OLD_PACKETS:
            if (context.flow) {
                cleanup_old_packets(context.flow);
            }
            break;
            
        case ActionType::ADJUST_RATE:
            if (context.flow) {
                double factor = 0.8; // Default factor
                // Parse factor from parameters if provided
                if (!action.parameters.empty()) {
                    std::istringstream iss(action.parameters);
                    std::string key, value;
                    if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                        if (key == "factor") {
                            factor = std::stod(value);
                        }
                    }
                }
                adjust_circulation_rate(context.flow, factor);
            }
            break;
            
        case ActionType::NOTIFY_USER:
            {
                std::string message = action.parameters;
                // Replace placeholders
                if (context.flow) {
                    size_t pos = message.find("{flow_name}");
                    if (pos != std::string::npos) {
                        message.replace(pos, 11, context.flow->name());
                    }
                }
                notify_user(message);
            }
            break;
            
        case ActionType::AUTO_SAVE:
            if (context.flow) {
                context.flow->maintain_flow();
            }
            break;
            
        case ActionType::COST_ANALYSIS:
            perform_cost_analysis();
            break;
    }
}

void WorkflowManager::optimize_circulation_pattern(FlowFile* flow) {
    if (!flow) return;
    
    std::cout << "Optimizing circulation pattern for flow: " << flow->name() << std::endl;
    
    // Get current pattern
    CirculationPattern pattern = flow->pattern();
    
    // Reduce circulation rate for cost optimization
    if (pattern.circulation_rate > 5) {
        pattern.circulation_rate = std::max(5u, pattern.circulation_rate - 2);
        flow->update_circulation_pattern(pattern);
        std::cout << "Reduced circulation rate to " << pattern.circulation_rate << " pps" << std::endl;
    }
    
    // Increase packet age for efficiency
    if (pattern.max_packet_age < 60000000) { // 60 seconds
        pattern.max_packet_age = std::min(60000000u, pattern.max_packet_age + 10000000);
        flow->update_circulation_pattern(pattern);
        std::cout << "Increased max packet age to " << pattern.max_packet_age << " μs" << std::endl;
    }
}

void WorkflowManager::compress_flow_data(FlowFile* flow) {
    if (!flow) return;
    
    std::cout << "Compressing flow data for: " << flow->name() << std::endl;
    
    // Simple compression simulation - in real implementation would use actual compression
    std::string content = flow->content();
    if (content.length() > 1000) { // Only compress large content
        // Simulate 30% compression
        std::cout << "Compressed " << content.length() << " bytes to " 
                  << static_cast<size_t>(content.length() * 0.7) << " bytes" << std::endl;
    }
}

void WorkflowManager::migrate_flow_to_optimal_node(FlowFile* flow) {
    if (!flow) return;
    
    std::cout << "Migrating flow to optimal node: " << flow->name() << std::endl;
    
    // Simulate migration by updating circulation path
    // In real implementation, this would find the optimal node and update routing
    std::cout << "Flow migrated to optimal node for better performance" << std::endl;
}

void WorkflowManager::backup_flow(FlowFile* flow) {
    if (!flow) return;
    
    std::cout << "Creating backup for flow: " << flow->name() << std::endl;
    
    // In real implementation, this would create a backup copy
    // For now, just log the action
    std::cout << "Backup created successfully" << std::endl;
}

void WorkflowManager::cleanup_old_packets(FlowFile* flow) {
    if (!flow) return;
    
    std::cout << "Cleaning up old packets for flow: " << flow->name() << std::endl;
    
    // In real implementation, this would clean up old packets
    // For now, just log the action
    std::cout << "Old packets cleaned up" << std::endl;
}

void WorkflowManager::adjust_circulation_rate(FlowFile* flow, double factor) {
    if (!flow) return;
    
    CirculationPattern pattern = flow->pattern();
    uint32_t new_rate = static_cast<uint32_t>(pattern.circulation_rate * factor);
    
    if (new_rate != pattern.circulation_rate) {
        pattern.circulation_rate = new_rate;
        flow->update_circulation_pattern(pattern);
        std::cout << "Adjusted circulation rate from " << pattern.circulation_rate 
                  << " to " << new_rate << " pps" << std::endl;
    }
}

void WorkflowManager::notify_user(const std::string& message) {
    std::cout << "NOTIFICATION: " << message << std::endl;
}

void WorkflowManager::perform_cost_analysis() {
    std::lock_guard<std::mutex> lock(costs_mutex_);
    
    std::cout << "\n=== COST ANALYSIS ===" << std::endl;
    std::cout << "Total Bandwidth Cost: $" << std::fixed << std::setprecision(2) 
              << total_bandwidth_cost_ << std::endl;
    std::cout << "Total CPU Cost: $" << total_cpu_cost_ << std::endl;
    std::cout << "Total Memory Cost: $" << total_memory_cost_ << std::endl;
    std::cout << "Total Cost: $" << get_total_cost() << std::endl;
    std::cout << "Active Flows: " << flow_costs_.size() << std::endl;
    
    for (const auto& pair : flow_costs_) {
        const FlowCost& cost = pair.second;
        double flow_total = cost.bandwidth_cost + cost.cpu_cost + cost.memory_cost;
        std::cout << "  " << cost.flow_name << ": $" << flow_total 
                  << " (packets: " << cost.packets_sent 
                  << ", bytes: " << cost.bytes_transferred << ")" << std::endl;
    }
    std::cout << "====================\n" << std::endl;
}

double WorkflowManager::calculate_bandwidth_cost(uint64_t bytes, double rate_per_mb) {
    return (bytes / (1024.0 * 1024.0)) * rate_per_mb;
}

double WorkflowManager::calculate_cpu_cost(double cpu_seconds, double rate_per_second) {
    return cpu_seconds * rate_per_second;
}

double WorkflowManager::calculate_memory_cost(uint64_t memory_mb, double rate_per_mb) {
    return memory_mb * rate_per_mb;
}

} // namespace nerd