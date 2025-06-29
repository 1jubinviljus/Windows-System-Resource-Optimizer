#ifndef PROCESS_OPTIMIZER_H
#define PROCESS_OPTIMIZER_H

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

// Structure to hold process information
typedef struct {
    DWORD processId;
    WCHAR process_name[MAX_PATH];
    double cpuUsage;
    SIZE_T memoryUsage;
    int priority;
} ProcessInfo;

// Function declarations for implemented functions
BOOL get_process_info(DWORD processId, ProcessInfo* info);
BOOL calculate_process_cpu_usage(DWORD processId, double* cpuUsage);
BOOL get_process_memory_usage(DWORD processId, SIZE_T* memoryUsage);
BOOL is_engineering_software(const WCHAR* processName);
BOOL adjust_process_priority(DWORD processId, int newPriority);
BOOL optimize_engineering_processes(void);

#endif // PROCESS_OPTIMIZER_H 