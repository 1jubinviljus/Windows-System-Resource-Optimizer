#ifndef PROCESS_OPTIMIZER_H
#define PROCESS_OPTIMIZER_H

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

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

#endif // PROCESS_OPTIMIZER_H 