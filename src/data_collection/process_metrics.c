#include "process_metrics.h"
#include <psapi.h>
#include <stdio.h>
#include <pdh.h>
#include <tlhelp32.h>
#include <stdbool.h>

// SQLite table creation statement
static const char *CREATE_PROCESS_TABLE = 
    "CREATE TABLE IF NOT EXISTS process_metrics ("
    "timestamp TEXT,"
    "process_id INTEGER,"
    "parent_process_id INTEGER,"
    "process_name TEXT,"
    "cpu_usage REAL,"
    "working_set_bytes INTEGER,"
    "private_bytes INTEGER,"
    "handle_count INTEGER,"
    "thread_count INTEGER,"
    "io_read_bytes INTEGER,"
    "io_write_bytes INTEGER,"
    "page_faults INTEGER"
    ");";

void get_process_info(DWORD process_id, ProcessInfo *info) {
    HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (process_handle == NULL) {
        return;
    }

    // Get basic process information
    info->process_id = process_id;
    
    // Get process name
    HMODULE module;
    DWORD needed;
    if (EnumProcessModules(process_handle, &module, sizeof(module), &needed)) {
        GetModuleBaseNameA(process_handle, module, info->process_name, sizeof(info->process_name));
    }
    
    // Get parent process ID using toolhelp32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(pe32);
        if (Process32FirstW(snapshot, &pe32)) {
            do {
                if (pe32.th32ProcessID == process_id) {
                    info->parent_process_id = pe32.th32ParentProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &pe32));
        }
        CloseHandle(snapshot);
    }

    // Get CPU usage
    info->cpu_usage = calculate_process_cpu(process_handle);

    // Get memory information
    get_process_memory_info(process_handle, info);

    // Get I/O information
    get_process_io_info(process_handle, info);

    // Get handle and thread count
    info->handle_count = 0;
    GetProcessHandleCount(process_handle, &info->handle_count);
    
    HANDLE snapshot_thread = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    info->thread_count = 0;
    if (snapshot_thread != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te32;
        te32.dwSize = sizeof(te32);
        if (Thread32First(snapshot_thread, &te32)) {
            do {
                if (te32.th32OwnerProcessID == process_id) {
                    info->thread_count++;
                }
            } while (Thread32Next(snapshot_thread, &te32));
        }
        CloseHandle(snapshot_thread);
    }

    CloseHandle(process_handle);
}

double calculate_process_cpu(HANDLE process) {
    static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    static DWORD lastProcessTime = 0;
    static SYSTEM_INFO sysInfo;
    static bool initialized = false;

    if (!initialized) {
        GetSystemInfo(&sysInfo);
        initialized = true;
    }

    ULARGE_INTEGER now, sys, user;
    DWORD currentTime = GetTickCount();
    double percent;

    GetSystemTimeAsFileTime((FILETIME*)&now);

    if (!GetProcessTimes(process, (FILETIME*)&now,
                        (FILETIME*)&now,
                        (FILETIME*)&sys,
                        (FILETIME*)&user)) {
        return 0.0;
    }

    if (!lastProcessTime) {
        lastCPU = now;
        lastSysCPU = sys;
        lastUserCPU = user;
        lastProcessTime = currentTime;
        return 0.0;
    }

    percent = ((sys.QuadPart - lastSysCPU.QuadPart) +
               (user.QuadPart - lastUserCPU.QuadPart)) * 100.0 /
              ((now.QuadPart - lastCPU.QuadPart) * sysInfo.dwNumberOfProcessors);

    lastCPU = now;
    lastSysCPU = sys;
    lastUserCPU = user;
    lastProcessTime = currentTime;

    return percent;
}

void get_process_memory_info(HANDLE process, ProcessInfo *info) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
        info->working_set = pmc.WorkingSetSize;
        info->private_bytes = pmc.PagefileUsage;
        info->page_faults = pmc.PageFaultCount;
    }
}

void get_process_io_info(HANDLE process, ProcessInfo *info) {
    IO_COUNTERS ioc;
    if (GetProcessIoCounters(process, &ioc)) {
        info->io_read_bytes = ioc.ReadTransferCount;
        info->io_write_bytes = ioc.WriteTransferCount;
    }
}

void collect_process_metrics(sqlite3 *db, const char *timestamp) {
    // Create table if it doesn't exist
    char *err_msg = NULL;
    if (sqlite3_exec(db, CREATE_PROCESS_TABLE, NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return;
    }

    // Get list of processes
    DWORD processes[1024], needed, process_count;
    if (!EnumProcesses(processes, sizeof(processes), &needed)) {
        return;
    }
    process_count = needed / sizeof(DWORD);

    // Prepare SQL statement
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO process_metrics VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return;
    }

    // Begin transaction
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Collect metrics for each process
    for (DWORD i = 0; i < process_count; i++) {
        if (processes[i] != 0) {
            ProcessInfo info = {0};
            get_process_info(processes[i], &info);

            // Only insert if we got valid process info
            if (info.process_name[0] != '\0') {
                sqlite3_bind_text(stmt, 1, timestamp, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, info.process_id);
                sqlite3_bind_int(stmt, 3, info.parent_process_id);
                sqlite3_bind_text(stmt, 4, info.process_name, -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 5, info.cpu_usage);
                sqlite3_bind_int64(stmt, 6, info.working_set);
                sqlite3_bind_int64(stmt, 7, info.private_bytes);
                sqlite3_bind_int(stmt, 8, info.handle_count);
                sqlite3_bind_int(stmt, 9, info.thread_count);
                sqlite3_bind_int64(stmt, 10, info.io_read_bytes);
                sqlite3_bind_int64(stmt, 11, info.io_write_bytes);
                sqlite3_bind_int(stmt, 12, info.page_faults);

                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
    }

    // Commit transaction and finalize statement
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
}
