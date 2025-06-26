#include "process_optimizer.h"
#include <windows.h>
#include <tlhelp32.h>

// Gets basic information about a process including its name and ID
BOOL get_process_info(DWORD processId, ProcessInfo* info) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) return FALSE;

    // Get process name
    if (!GetProcessImageFileNameW(hProcess, info->process_name, MAX_PATH)) {
        CloseHandle(hProcess);
        return FALSE;
    }
    info->process_id = processId;

    // Optionally: fill out more fields (memory, CPU, etc.)

    CloseHandle(hProcess);
    return TRUE;
}

// Calculates the total CPU time used by a process in seconds
BOOL calculate_process_cpu_usage(DWORD processId, double* cpuUsage) {
    if (cpuUsage == NULL) return FALSE;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) return FALSE;

    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (!GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
        CloseHandle(hProcess);
        return FALSE;
    }
    ULARGE_INTEGER kernelTimeUL, userTimeUL;
    kernelTimeUL.LowPart = kernelTime.dwLowDateTime;
    kernelTimeUL.HighPart = kernelTime.dwHighDateTime;
    userTimeUL.LowPart = userTime.dwLowDateTime;
    userTimeUL.HighPart = userTime.dwHighDateTime;
    ULONGLONG totalTime = kernelTimeUL.QuadPart + userTimeUL.QuadPart;
    *cpuUsage = (double)totalTime / 10000000.0;
    CloseHandle(hProcess);
    return TRUE;
}

BOOL get_process_memory_usage(DWORD processId, SIZE_T* memoryUsage) { 
    if (memoryUsage == NULL) return FALSE;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) return FALSE;
    PROCESS_MEMORY_COUNTERS pmc;
    pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
    if (!GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        CloseHandle(hProcess);
        return FALSE;
    }
    *memoryUsage = pmc.WorkingSetSize;
    CloseHandle(hProcess);
    return TRUE;    
}

BOOL is_engineering_software(const WCHAR* processName) {
    if (processName == NULL) return FALSE;
    const WCHAR* engineeringSoftware[] = {
        L"solidworks.exe",
        L"autocad.exe",
        L"matlab.exe",
        L"ansys.exe",
        L"inventor.exe",
        L"catia.exe",
        L"nx.exe",
        L"simulink.exe",
        NULL
    };
    for (int i = 0; engineeringSoftware[i] != NULL; i++) {
        if (_wcsicmp(processName, engineeringSoftware[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL adjust_process_priority(DWORD processId, int newPriority) {
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processId);
    if (hProcess == NULL) return FALSE;
    BOOL result = SetPriorityClass(hProcess, newPriority);
    CloseHandle(hProcess);
    return result;
}

// Main optimization function - implement this last
BOOL optimize_engineering_processes(void) {
    BOOL optimized_any = FALSE;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (!Process32FirstW(hSnapshot, &pe)) {
        CloseHandle(hSnapshot);
        return FALSE;
    }
    do {
        if (is_engineering_software(pe.szExeFile)) {
            if (adjust_process_priority(pe.th32ProcessID, HIGH_PRIORITY_CLASS)) {
                optimized_any = TRUE;
                wprintf(L"Optimized: %s (PID: %lu)\n", pe.szExeFile, pe.th32ProcessID);
            }
        }
    } while (Process32NextW(hSnapshot, &pe));
    CloseHandle(hSnapshot);
    return optimized_any;
}
