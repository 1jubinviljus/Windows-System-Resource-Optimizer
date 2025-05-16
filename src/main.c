#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <sqlite3.h>
#include <time.h>

// Function declarations
void create_table(sqlite3 *db);
void insert_data(sqlite3 *db, const char *timestamp, double cpu_usage, double mem_usage, double disk_usage);
void get_system_stats(double *mem, double *disk);
void get_timestamp(char *buffer, size_t size);
double get_cpu_usage();

int main() {
    sqlite3 *db;

    // Open or create SQLite database
    if (sqlite3_open("build/optimizer.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Create table if not exists
    create_table(db);

    //Program start time
    time_t start_time = time(NULL);

    //Run the program for 60 seconds
    while(difftime(time(NULL),start_time) <60 ){
        // Gather system statistics
        char timestamp[32];
        double cpu, mem, disk;

        get_timestamp(timestamp, sizeof(timestamp));
        cpu = get_cpu_usage();
        get_system_stats(&mem, &disk); // We already got CPU separately

        // Insert collected data into database
        insert_data(db, timestamp, cpu, mem, disk);

        // Feedback
        printf("System stats collected and inserted:\n");
        printf("Timestamp: %s\nCPU: %.2f%%\nMemory: %.2f%%\nDisk: %.2f%%\n",
            timestamp, cpu, mem, disk);

        Sleep(2000); //collect data every 2 seconds
    }

    sqlite3_close(db);
    return 0;
}

// Creates the system_stats table
void create_table(sqlite3 *db) {
    const char *sql = "CREATE TABLE IF NOT EXISTS system_stats("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "timestamp TEXT, "
                      "cpu_usage REAL, "
                      "memory_usage REAL, "
                      "disk_usage REAL);";

    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error in create_table: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

// Inserts system stats into the database
void insert_data(sqlite3 *db, const char *timestamp, double cpu_usage, double mem_usage, double disk_usage) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT INTO system_stats (timestamp, cpu_usage, memory_usage, disk_usage) "
             "VALUES ('%s', %f, %f, %f);",
             timestamp, cpu_usage, mem_usage, disk_usage);

    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error in insert_data: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

// Gets memory and disk usage
void get_system_stats(double *mem, double *disk) {
    // Memory usage
    if (mem) {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            *mem = (double)memInfo.dwMemoryLoad;
        } else {
            *mem = -1.0;
        }
    }

    // Disk usage
    if (disk) {
        ULARGE_INTEGER freeBytes, totalBytes;
        if (GetDiskFreeSpaceEx("C:\\", NULL, &totalBytes, &freeBytes)) {
            *disk = 100.0 - ((double)freeBytes.QuadPart / (double)totalBytes.QuadPart) * 100;
        } else {
            *disk = -1.0;
        }
    }
}

// Gets current timestamp in readable format
void get_timestamp(char *buffer, size_t size) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

// Measures CPU usage using two samples
double get_cpu_usage() {
    FILETIME idleTime1, kernelTime1, userTime1;
    FILETIME idleTime2, kernelTime2, userTime2;
    ULARGE_INTEGER idle1, kernel1, user1;
    ULARGE_INTEGER idle2, kernel2, user2;

    if (!GetSystemTimes(&idleTime1, &kernelTime1, &userTime1)) return -1.0;
    Sleep(100);  // 100 ms delay
    if (!GetSystemTimes(&idleTime2, &kernelTime2, &userTime2)) return -1.0;

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

    ULONGLONG idleDiff = idle2.QuadPart - idle1.QuadPart;
    ULONGLONG kernelDiff = kernel2.QuadPart - kernel1.QuadPart;
    ULONGLONG userDiff = user2.QuadPart - user1.QuadPart;
    ULONGLONG total = kernelDiff + userDiff;

    if (total == 0) return -1.0;

    double cpuUsage = (1.0 - ((double)idleDiff / total)) * 100.0;
    return cpuUsage;
}
