#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <sqlite3.h>

// Function declarations
void create_table(sqlite3 *db);
void insert_data(sqlite3 *db, const char *timestamp, double cpu_usage, double mem_usage, double disk_usage);
void get_system_stats(double *cpu, double *mem, double *disk);
void get_timestamp(char *buffer, size_t size);

int main() {
    sqlite3 *db;

    // Open or create SQLite database
    if (sqlite3_open("build/optimizer.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Create table if not exists
    create_table(db);

    // Gather system statistics
    char timestamp[32];
    double cpu, mem, disk;

    get_timestamp(timestamp, sizeof(timestamp));
    get_system_stats(&cpu, &mem, &disk);

    // Insert collected data into database
    insert_data(db, timestamp, cpu, mem, disk);

    // Feedback
    printf("System stats collected and inserted:\n");
    printf("Timestamp: %s\nCPU: %.2f%%\nMemory: %.2f%%\nDisk: %.2f%%\n",
           timestamp, cpu, mem, disk);

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

// Gets system resource usage (memory, disk, placeholder CPU)
void get_system_stats(double *cpu, double *mem, double *disk) {
    // Memory usage
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        *mem = (double)memInfo.dwMemoryLoad;
    } else {
        *mem = -1.0;
    }

    // Disk usage
    ULARGE_INTEGER freeBytes, totalBytes;
    if (GetDiskFreeSpaceEx("C:\\", NULL, &totalBytes, &freeBytes)) {
        *disk = 100.0 - ((double)freeBytes.QuadPart / (double)totalBytes.QuadPart) * 100;
    } else {
        *disk = -1.0;
    }

    // CPU usage placeholder (implementing real CPU usage tracking requires time sampling)
    *cpu = -1.0;
}

// Gets current timestamp in readable format
void get_timestamp(char *buffer, size_t size) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}
