#ifndef PROCESS_METRICS_H
#define PROCESS_METRICS_H

#include <windows.h>
#include <sqlite3.h>

// Structure to hold process information
typedef struct {
    DWORD process_id;
    DWORD parent_process_id;
    char process_name[MAX_PATH];
    double cpu_usage;
    SIZE_T working_set;
    SIZE_T private_bytes;
    DWORD handle_count;
    DWORD thread_count;
    ULONGLONG io_read_bytes;
    ULONGLONG io_write_bytes;
    DWORD page_faults;
} ProcessInfo;

// Main collection function called from main.c
void collect_process_metrics(sqlite3 *db, const char *timestamp);

// Process information gathering
void get_process_info(DWORD process_id, ProcessInfo *info);
double calculate_process_cpu(HANDLE process);
void get_process_memory_info(HANDLE process, ProcessInfo *info);
void get_process_io_info(HANDLE process, ProcessInfo *info);

#endif // PROCESS_METRICS_H