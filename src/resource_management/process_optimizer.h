#ifndef PROCESS_OPTIMIZER_H
#define PROCESS_OPTIMIZER_H

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdbool.h>
#include <sqlite3.h>
#include "../data_collection/process_metrics.h"
#include "../data_collection/system_metrics.h"

// Structure to hold process information
typedef struct {
    DWORD processId;
    WCHAR name[MAX_PATH];
    double cpuUsage;
    SIZE_T memoryUsage;
    int priority;
} ProcessInfo;

// Structure to hold system resource information
typedef struct {
    double cpuUsage;
    double memoryUsage;
    double diskQueueLength;
    DWORD largeBlockCount;
    double fragmentationPercent;
} SystemResources;

// Optimization thresholds
#define PROCESS_CPU_HIGH_THRESHOLD    75.0  // CPU % that triggers optimization
#define PROCESS_MEM_HIGH_THRESHOLD    80.0  // Memory % that triggers optimization
#define SYSTEM_CPU_PRESSURE_THRESHOLD 85.0  // System-wide CPU pressure threshold
#define SYSTEM_MEM_PRESSURE_THRESHOLD 90.0  // System-wide memory pressure threshold
#define OPTIMIZATION_COOLDOWN_MS      300000 // 5 minutes between optimizations
#define MAX_OPTIMIZATION_ATTEMPTS     3     // Maximum times to optimize a process

// Exercise 1: Basic Process Functions
BOOL get_process_info(DWORD processId, ProcessInfo* info);
BOOL calculate_process_cpu_usage(DWORD processId, double* cpuUsage);
BOOL get_process_memory_usage(DWORD processId, SIZE_T* memoryUsage);

// Exercise 2: System Resource Monitoring
BOOL get_system_resources(SystemResources* resources);
BOOL is_engineering_software(const WCHAR* processName);
BOOL adjust_process_priority(DWORD processId, int newPriority);

// Exercise 3: Optimization Functions
BOOL optimize_engineering_processes(void);
BOOL check_memory_fragmentation(void);

/**
 * @brief Optimizes a specific process based on its resource usage and type
 * 
 * @param process_id The ID of the process to optimize
 * @param process_info Current metrics and information about the process
 * @param sys_metrics Current system-wide metrics
 * @return bool True if optimization was applied successfully
 */
bool optimize_process(DWORD process_id, const ProcessInfo* process_info, const SystemMetrics* sys_metrics);

/**
 * @brief Resets the optimization history for all processes
 * This is useful when system state has changed significantly
 */
void reset_optimization_history(void);

/**
 * @brief Main optimization function that should be called periodically
 * Reads current metrics from the database and optimizes processes as needed
 * 
 * @param db Pointer to the SQLite database containing metrics
 */
void optimize_system_performance(sqlite3* db);

/**
 * @brief Checks if a process is an engineering application
 * 
 * @param process_name Name of the process executable
 * @return bool True if the process is a known engineering application
 */
bool is_engineering_app(const wchar_t* process_name);

/**
 * @brief Gets the recommended CPU priority for a process
 * 
 * @param process_info Process metrics and information
 * @param is_engineering Whether the process is an engineering application
 * @return DWORD Windows priority class value
 */
DWORD get_recommended_priority(const ProcessInfo* process_info, bool is_engineering);

/**
 * @brief Adjusts memory settings for a process
 * 
 * @param process_handle Handle to the target process
 * @param working_set_size Current working set size
 * @return bool True if adjustments were made successfully
 */
bool adjust_process_memory(HANDLE process_handle, SIZE_T working_set_size);

#endif // PROCESS_OPTIMIZER_H 