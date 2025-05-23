#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <sqlite3.h>
#include <time.h>

// Function declarations
void create_tables(sqlite3 *db);
void insert_system_data(sqlite3 *db, const char *timestamp, double cpu_usage, double mem_usage, double disk_usage);
void insert_process_data(sqlite3 *db, const char *timestamp);
void get_system_stats(double *cpu, double *mem, double *disk);
void get_timestamp(char *buffer, size_t size);
double get_cpu_usage();

int main() {
    sqlite3 *db;
    if (sqlite3_open("build/optimizer.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    create_tables(db);
    time_t start_time = time(NULL);

    while (difftime(time(NULL), start_time) < 60) {
        char timestamp[32];
        double cpu, mem, disk;

        get_timestamp(timestamp, sizeof(timestamp));
        cpu = get_cpu_usage();
        get_system_stats(NULL, &mem, &disk);

        insert_system_data(db, timestamp, cpu, mem, disk);
        insert_process_data(db, timestamp);

        printf("[Inserted] %s | CPU: %.2f%% | Mem: %.2f%% | Disk: %.2f%%\n", timestamp, cpu, mem, disk);
        Sleep(2000);
    }

    sqlite3_close(db);
    return 0;
}

void create_tables(sqlite3 *db) {
    const char *system_sql =
        "CREATE TABLE IF NOT EXISTS system_stats("  
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp TEXT, "
        "cpu_usage REAL, "
        "memory_usage REAL, "
        "disk_usage REAL);";

    const char *process_sql =
        "CREATE TABLE IF NOT EXISTS process_stats("  
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp TEXT, "
        "process_name TEXT, "
        "memory_usage_kb INTEGER, "
        "cpu_usage_percent REAL);";

    char *err_msg = 0;
    if (sqlite3_exec(db, system_sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error (system_stats): %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    if (sqlite3_exec(db, process_sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error (process_stats): %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

void insert_system_data(sqlite3 *db, const char *timestamp, double cpu, double mem, double disk) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO system_stats (timestamp, cpu_usage, memory_usage, disk_usage) "
        "VALUES ('%s', %f, %f, %f);", timestamp, cpu, mem, disk);

    char *err_msg = 0;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error in insert_system_data: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

void insert_process_data(sqlite3 *db, const char *timestamp) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE) return;

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return;
    }

    do {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            PROCESS_MEMORY_COUNTERS pmc;
            FILETIME ftCreation, ftExit, ftKernel1, ftUser1, ftKernel2, ftUser2;

            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)) &&
                GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel1, &ftUser1)) {

                Sleep(100);

                if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel2, &ftUser2)) {
                    ULARGE_INTEGER k1, u1, k2, u2;
                    k1.LowPart = ftKernel1.dwLowDateTime; k1.HighPart = ftKernel1.dwHighDateTime;
                    u1.LowPart = ftUser1.dwLowDateTime; u1.HighPart = ftUser1.dwHighDateTime;
                    k2.LowPart = ftKernel2.dwLowDateTime; k2.HighPart = ftKernel2.dwHighDateTime;
                    u2.LowPart = ftUser2.dwLowDateTime; u2.HighPart = ftUser2.dwHighDateTime;

                    ULONGLONG deltaTime = (k2.QuadPart - k1.QuadPart) + (u2.QuadPart - u1.QuadPart);
                    double cpuUsagePercent = deltaTime / (100000.0); // Normalize over 100ms interval

                    char sql[512];
                    snprintf(sql, sizeof(sql),
                        "INSERT INTO process_stats (timestamp, process_name, memory_usage_kb, cpu_usage_percent) "
                        "VALUES ('%s', '%s', %lu, %.2f);",
                        timestamp, pe32.szExeFile, pmc.WorkingSetSize / 1024, cpuUsagePercent);

                    char *err_msg = 0;
                    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
                        fprintf(stderr, "SQL error in insert_process_data: %s\n", err_msg);
                        sqlite3_free(err_msg);
                    }
                }
            }
            CloseHandle(hProcess);
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}

void get_system_stats(double *cpu, double *mem, double *disk) {
    if (mem) {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        *mem = GlobalMemoryStatusEx(&memInfo) ? (double)memInfo.dwMemoryLoad : -1.0;
    }

    if (disk) {
        ULARGE_INTEGER freeBytes, totalBytes;
        if (GetDiskFreeSpaceEx("C:\\", NULL, &totalBytes, &freeBytes)) {
            *disk = 100.0 - ((double)freeBytes.QuadPart / (double)totalBytes.QuadPart) * 100;
        } else {
            *disk = -1.0;
        }
    }
}

void get_timestamp(char *buffer, size_t size) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

double get_cpu_usage() {
    FILETIME idleTime1, kernelTime1, userTime1;
    FILETIME idleTime2, kernelTime2, userTime2;
    ULARGE_INTEGER idle1, kernel1, user1;
    ULARGE_INTEGER idle2, kernel2, user2;

    // Take first snapshot
    if (!GetSystemTimes(&idleTime1, &kernelTime1, &userTime1)) return -1.0;

    Sleep(100);  // Wait 100 ms

    // Take second snapshot
    if (!GetSystemTimes(&idleTime2, &kernelTime2, &userTime2)) return -1.0;

    // Convert FILETIMEs to ULARGE_INTEGERs
    idle1.LowPart = idleTime1.dwLowDateTime;
    idle1.HighPart = idleTime1.dwHighDateTime;
    kernel1.LowPart = kernelTime1.dwLowDateTime;
    kernel1.HighPart = kernelTime1.dwHighDateTime;
    user1.LowPart = userTime1.dwLowDateTime;
    user1.HighPart = userTime1.dwHighDateTime;

    idle2.LowPart = idleTime2.dwLowDateTime;
    idle2.HighPart = idleTime2.dwHighDateTime;
    kernel2.LowPart = kernelTime2.dwLowDateTime;
    kernel2.HighPart = kernelTime2.dwHighDateTime;
    user2.LowPart = userTime2.dwLowDateTime;
    user2.HighPart = userTime2.dwHighDateTime;

    // Calculate deltas
    ULONGLONG idleDiff = idle2.QuadPart - idle1.QuadPart;
    ULONGLONG kernelDiff = kernel2.QuadPart - kernel1.QuadPart;
    ULONGLONG userDiff = user2.QuadPart - user1.QuadPart;
    ULONGLONG total = kernelDiff + userDiff;

    // Avoid divide-by-zero
    if (total == 0) return -1.0;

    // Calculate CPU usage
    double usage = (1.0 - ((double)idleDiff / total)) * 100.0;
    return usage;
}

