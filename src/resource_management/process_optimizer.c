#include "process_optimizer.h"
#include <stdio.h>

/*
Exercise 1: Implement get_process_info
This function should:
1. Open the process using OpenProcess
2. Get the process name
3. Get CPU and memory usage
4. Return TRUE if successful, FALSE if failed

Tips:
- Use OpenProcess() to get process handle
- Use GetProcessImageFileNameW() for process name
- Check for NULL handles
- Remember to CloseHandle() when done
*/
BOOL get_process_info(DWORD processId, ProcessInfo* info) {
    if (info == NULL) return FALSE;
    
    // TODO: Implement this function
    // 1. Open the process
    // 2. Get process information
    // 3. Clean up handles
    
    return TRUE;
}

/*
Exercise 2: Implement calculate_process_cpu_usage
This function should:
1. Get process times (kernel and user)
2. Calculate CPU usage percentage
3. Return TRUE if successful, FALSE if failed

Tips:
- Use GetProcessTimes()
- Calculate difference between two measurements
- Remember system has multiple CPU cores
*/
BOOL calculate_process_cpu_usage(DWORD processId, double* cpuUsage) {
    if (cpuUsage == NULL) return FALSE;
    
    // TODO: Implement this function
    // 1. Get process handle
    // 2. Get process times
    // 3. Calculate usage
    
    return TRUE;
}

/*
Exercise 3: Implement get_process_memory_usage
This function should:
1. Get process memory counters
2. Store working set size
3. Return TRUE if successful, FALSE if failed

Tips:
- Use GetProcessMemoryInfo()
- Look at working set size
- Consider private bytes too
*/
BOOL get_process_memory_usage(DWORD processId, SIZE_T* memoryUsage) {
    if (memoryUsage == NULL) return FALSE;
    
    // TODO: Implement this function
    // 1. Open process
    // 2. Get memory info
    // 3. Store results
    
    return TRUE;
}

/*
Exercise 4: Implement is_engineering_software
This function should:
1. Check if process name matches known engineering software
2. Return TRUE if it is engineering software, FALSE if not

Tips:
- Check for common names like "solidworks.exe", "autocad.exe"
- Use case-insensitive comparison
- Consider using wcscmp() or similar
*/
BOOL is_engineering_software(const WCHAR* processName) {
    if (processName == NULL) return FALSE;
    
    // TODO: Implement this function
    // 1. Define engineering software names
    // 2. Compare process name
    
    return FALSE;
}

/*
Exercise 5: Implement adjust_process_priority
This function should:
1. Open the process
2. Set new priority class
3. Return TRUE if successful, FALSE if failed

Tips:
- Use SetPriorityClass()
- Check for permissions
- Handle errors appropriately
*/
BOOL adjust_process_priority(DWORD processId, int newPriority) {
    // TODO: Implement this function
    // 1. Open process with appropriate rights
    // 2. Set priority
    // 3. Handle errors
    
    return TRUE;
}

// Main optimization function - implement this last
BOOL optimize_engineering_processes(void) {
    // TODO: Implement this function
    // 1. Enumerate all processes
    // 2. Find engineering software
    // 3. Optimize their priorities
    
    return TRUE;
} 