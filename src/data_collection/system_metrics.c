#include <stdio.h>
#include <pdh.h>
#include <psapi.h>
#include "system_metrics.h"
#include <winevt.h>    // For Windows Event Log
#include <windows.h>
#include <tlhelp32.h>    // For thread enumeration
#include <pdhmsg.h>
#include <time.h>

// Add the libraries we need to link against
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wevtapi.lib")  // For Event Log

// Global PDH (Performance Data Helper) variables
static PDH_HQUERY cpuQuery;
static PDH_HQUERY diskQuery;
static PDH_HQUERY networkQuery;
static PDH_HCOUNTER cpuTotal;
static PDH_HCOUNTER diskRead;
static PDH_HCOUNTER diskWrite;
static PDH_HCOUNTER networkIn;
static PDH_HCOUNTER networkOut;

// Add these new structures after your existing global variables
typedef struct {
    DWORD crash_count;
    DWORD error_count;
    DWORD warning_count;
} SystemEventCounts;

typedef struct {
    double read_latency;
    double write_latency;
    DWORD split_io_count;
} DiskHealthInfo;

typedef struct {
    SIZE_T total_free;
    SIZE_T largest_free;
    double fragmentation_percent;
} MemoryFragInfo;

// Error handling helper
static void log_error(const char* function_name, const char* error_msg) {
    fprintf(stderr, "Error in %s: %s\n", function_name, error_msg);
}

// Initialize the PDH counters
static BOOL initialize_counters(void) {
    // CPU Counters
    if (PdhOpenQuery(NULL, 0, &cpuQuery) != ERROR_SUCCESS) {
        log_error("initialize_counters", "Failed to open CPU query");
        return FALSE;
    }
    if (PdhAddCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &cpuTotal) != ERROR_SUCCESS) {
        log_error("initialize_counters", "Failed to add CPU counter");
        return FALSE;
    }
    PdhCollectQueryData(cpuQuery);

    // Disk Counters
    if (PdhOpenQuery(NULL, 0, &diskQuery) != ERROR_SUCCESS) {
        log_error("initialize_counters", "Failed to open disk query");
        return FALSE;
    }
    if (PdhAddCounterW(diskQuery, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &diskRead) != ERROR_SUCCESS) {
        log_error("initialize_counters", "Failed to add disk read counter");
        return FALSE;
    }
    if (PdhAddCounterW(diskQuery, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &diskWrite) != ERROR_SUCCESS) {
        log_error("initialize_counters", "Failed to add disk write counter");
        return FALSE;
    }
    PdhCollectQueryData(diskQuery);
    
    // Network Counters
    if (PdhOpenQuery(NULL, 0, &networkQuery) != ERROR_SUCCESS) {
        log_error("initialize_counters", "Failed to open network query");
        return FALSE;
    }
    if (PdhAddCounterW(networkQuery, L"\\Network Interface(*)\\Bytes Received/sec", 0, &networkIn) != ERROR_SUCCESS) {
        log_error("initialize_counters", "Failed to add network in counter");
        return FALSE;
    }
    if (PdhAddCounterW(networkQuery, L"\\Network Interface(*)\\Bytes Sent/sec", 0, &networkOut) != ERROR_SUCCESS) {
        log_error("initialize_counters", "Failed to add network out counter");
        return FALSE;
    }
    PdhCollectQueryData(networkQuery);

    return TRUE;
}

double get_cpu_usage(void) {
    PDH_FMT_COUNTERVALUE counterVal;
    
    if (PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS) {
        log_error("get_cpu_usage", "Failed to collect CPU data");
        return -1.0;
    }
    
    if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) != ERROR_SUCCESS) {
        log_error("get_cpu_usage", "Failed to format CPU counter");
        return -1.0;
    }
    
    return counterVal.doubleValue;
}

double get_memory_usage(void) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo) == 0) {
        log_error("get_memory_usage", "Failed to get memory status");
        return -1.0;
    }

    return (double)memInfo.dwMemoryLoad;
}

double get_page_file_usage(void) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&memInfo) == 0) {
        log_error("get_page_file_usage", "Failed to get memory status");
        return -1.0;
    }

    return ((double)(memInfo.ullTotalPageFile - memInfo.ullAvailPageFile) / 
            memInfo.ullTotalPageFile) * 100.0;
}

void get_disk_metrics(double *read_bytes, double *write_bytes, int *queue_length) {
    PDH_FMT_COUNTERVALUE readVal, writeVal, queueVal;
    static PDH_HCOUNTER queueCounter = NULL;
    
    // Initialize return values to -1 in case of error
    *read_bytes = -1.0;
    *write_bytes = -1.0;
    *queue_length = -1;
    
    // Add queue counter if not already added
    if (queueCounter == NULL) {
        if (PdhAddCounterW(diskQuery, L"\\PhysicalDisk(_Total)\\Current Disk Queue Length",
                         0, &queueCounter) != ERROR_SUCCESS) {
            log_error("get_disk_metrics", "Failed to add queue counter");
            return;
        }
    }
    
    if (PdhCollectQueryData(diskQuery) != ERROR_SUCCESS) {
        log_error("get_disk_metrics", "Failed to collect disk data");
        return;
    }
    
    if (PdhGetFormattedCounterValue(diskRead, PDH_FMT_DOUBLE, NULL, &readVal) != ERROR_SUCCESS) {
        log_error("get_disk_metrics", "Failed to get read value");
        return;
    }
    
    if (PdhGetFormattedCounterValue(diskWrite, PDH_FMT_DOUBLE, NULL, &writeVal) != ERROR_SUCCESS) {
        log_error("get_disk_metrics", "Failed to get write value");
        return;
    }
    
    if (PdhGetFormattedCounterValue(queueCounter, PDH_FMT_LONG, NULL, &queueVal) != ERROR_SUCCESS) {
        log_error("get_disk_metrics", "Failed to get queue length");
        return;
    }
    
    *read_bytes = readVal.doubleValue;
    *write_bytes = writeVal.doubleValue;
    *queue_length = queueVal.longValue;
}

void get_network_metrics(double *bytes_in, double *bytes_out) {
    PDH_FMT_COUNTERVALUE inVal, outVal;
    
    // Initialize return values to -1 in case of error
    *bytes_in = -1.0;
    *bytes_out = -1.0;
    
    if (PdhCollectQueryData(networkQuery) != ERROR_SUCCESS) {
        log_error("get_network_metrics", "Failed to collect network data");
        return;
    }
    
    if (PdhGetFormattedCounterValue(networkIn, PDH_FMT_DOUBLE, NULL, &inVal) != ERROR_SUCCESS) {
        log_error("get_network_metrics", "Failed to get bytes received");
        return;
    }
    
    if (PdhGetFormattedCounterValue(networkOut, PDH_FMT_DOUBLE, NULL, &outVal) != ERROR_SUCCESS) {
        log_error("get_network_metrics", "Failed to get bytes sent");
        return;
    }
    
    *bytes_in = inVal.doubleValue;
    *bytes_out = outVal.doubleValue;
}

void get_system_events(void) {
    EVT_HANDLE hEventLog = NULL;
    EVT_HANDLE hEvents[10] = { 0 };  // Array to store event handles
    DWORD dwReturned = 0;
    
    // Open the System event log
    hEventLog = EvtOpenLog(NULL, L"System", EvtOpenChannelPath);
    if (hEventLog == NULL) {
        log_error("get_system_events", "Failed to open System event log");
        return;
    }

    // Query for error events in the last hour
    LPCWSTR pwsQuery = L"*[System[(Level<=2) and TimeCreated[timediff(@SystemTime) <= 3600000]]]";
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", pwsQuery, EvtQueryChannelPath);
    
    if (hResults == NULL) {
        log_error("get_system_events", "Failed to query events");
        EvtClose(hEventLog);
        return;
    }

    // Get the most recent events
    if (!EvtNext(hResults, 10, hEvents, INFINITE, 0, &dwReturned)) {
        if (GetLastError() != ERROR_NO_MORE_ITEMS) {
            log_error("get_system_events", "Failed to get events");
        }
    }

    // Clean up
    for (DWORD i = 0; i < dwReturned; i++) {
        EvtClose(hEvents[i]);
    }
    EvtClose(hResults);
    EvtClose(hEventLog);
}

double get_cpu_temperature(void) {
    // Note: This requires WMI (Windows Management Instrumentation)
    // This is a placeholder - implement with WMI for actual CPU temperature
    return 0.0;
}

SystemEventCounts get_system_event_counts(void) {
    SystemEventCounts counts = {0};
    EVT_HANDLE hEventLog = NULL;
    
    // Open the System event log
    hEventLog = EvtOpenLog(NULL, L"System", EvtOpenChannelPath);
    if (NULL == hEventLog) {
        log_error("get_system_event_counts", "Failed to open System event log");
        return counts;
    }

    // Query strings for different event types
    LPCWSTR queryStrings[] = {
        L"*[System[(Level=1)]]",  // Critical/Crashes
        L"*[System[(Level=2)]]",  // Errors
        L"*[System[(Level=3)]]"   // Warnings
    };

    for (int i = 0; i < 3; i++) {
        EVT_HANDLE hResults = EvtQuery(NULL, L"System", queryStrings[i], EvtQueryChannelPath);
        if (hResults) {
            DWORD returned = 0;
            EvtNext(hResults, 1, NULL, 0, 0, &returned);
            
            switch(i) {
                case 0: counts.crash_count = returned; break;
                case 1: counts.error_count = returned; break;
                case 2: counts.warning_count = returned; break;
            }
            
            EvtClose(hResults);
        }
    }

    EvtClose(hEventLog);
    return counts;
}

DiskHealthInfo get_disk_health(void) {
    DiskHealthInfo info = {0};
    PDH_HQUERY query;
    PDH_HCOUNTER readTime, writeTime, splitIO;
    
    if (PdhOpenQuery(NULL, 0, &query) == ERROR_SUCCESS) {
        PdhAddCounterW(query, L"\\PhysicalDisk(_Total)\\Avg. Disk sec/Read", 0, &readTime);
        PdhAddCounterW(query, L"\\PhysicalDisk(_Total)\\Avg. Disk sec/Write", 0, &writeTime);
        PdhAddCounterW(query, L"\\PhysicalDisk(_Total)\\Split IO/Sec", 0, &splitIO);
        
        PdhCollectQueryData(query);
        Sleep(1000);  // Wait for a second sample
        PdhCollectQueryData(query);
        
        PDH_FMT_COUNTERVALUE value;
        PdhGetFormattedCounterValue(readTime, PDH_FMT_DOUBLE, NULL, &value);
        info.read_latency = value.doubleValue * 1000.0;  // Convert to milliseconds
        
        PdhGetFormattedCounterValue(writeTime, PDH_FMT_DOUBLE, NULL, &value);
        info.write_latency = value.doubleValue * 1000.0;  // Convert to milliseconds
        
        PdhGetFormattedCounterValue(splitIO, PDH_FMT_LONG, NULL, &value);
        info.split_io_count = value.longValue;
        
        PdhCloseQuery(query);
    }
    
    return info;
}

MemoryFragInfo get_memory_fragmentation(void) {
    MemoryFragInfo info = {0};
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    MEMORY_BASIC_INFORMATION memInfo;
    LPVOID addr = 0;
    SIZE_T totalFree = 0;
    SIZE_T largestFree = 0;
    SIZE_T freeRegions = 0;
    
    while (VirtualQuery(addr, &memInfo, sizeof(memInfo))) {
        if (memInfo.State == MEM_FREE) {
            totalFree += memInfo.RegionSize;
            largestFree = max(largestFree, memInfo.RegionSize);
            freeRegions++;
        }
        addr = (LPVOID)((DWORD_PTR)memInfo.BaseAddress + memInfo.RegionSize);
        if (addr >= (LPVOID)((DWORD_PTR)sysInfo.lpMaximumApplicationAddress)) {
            break;
        }
    }
    
    info.total_free = totalFree;
    info.largest_free = largestFree;
    info.fragmentation_percent = freeRegions > 1 ? 
        (1.0 - ((double)largestFree / totalFree)) * 100.0 : 0.0;
    
    return info;
}

void collect_system_metrics(sqlite3 *db, const char *timestamp) {
    static BOOL initialized = FALSE;
    char sql[2048];  // Increased buffer size for more metrics
    char *err_msg = 0;
    
    // Initialize counters if first time
    if (!initialized) {
        if (!initialize_counters()) {
            log_error("collect_system_metrics", "Failed to initialize counters");
            return;
        }
        initialized = TRUE;
    }
    
    // Collect basic metrics
    double cpu = get_cpu_usage();
    double cpu_temp = get_cpu_temperature();
    double mem = get_memory_usage();
    double page_file = get_page_file_usage();
    
    double disk_read, disk_write;
    int disk_queue;
    get_disk_metrics(&disk_read, &disk_write, &disk_queue);
    
    double net_in, net_out;
    get_network_metrics(&net_in, &net_out);
    
    // Collect advanced metrics
    SystemEventCounts eventCounts = get_system_event_counts();
    DiskHealthInfo diskHealth = get_disk_health();
    MemoryFragInfo memFrag = get_memory_fragmentation();
    
    // Create SQL insert statement with all metrics
    snprintf(sql, sizeof(sql),
        "INSERT INTO system_metrics ("
        "timestamp, cpu_usage, cpu_temperature, memory_usage, "
        "page_file_usage, disk_read_bytes, disk_write_bytes, "
        "disk_queue_length, network_bytes_in, network_bytes_out, "
        "crash_count, error_count, warning_count, "
        "disk_read_latency, disk_write_latency, disk_split_io, "
        "memory_fragmentation, memory_largest_free, memory_total_free"
        ") VALUES ("
        "'%s', %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %d, %.2f, %.2f, "
        "%lu, %lu, %lu, %.2f, %.2f, %lu, %.2f, %llu, %llu"
        ");",
        timestamp, cpu, cpu_temp, mem, page_file,
        disk_read, disk_write, disk_queue, net_in, net_out,
        eventCounts.crash_count, eventCounts.error_count, eventCounts.warning_count,
        diskHealth.read_latency, diskHealth.write_latency, diskHealth.split_io_count,
        memFrag.fragmentation_percent, 
        (unsigned long long)memFrag.largest_free, 
        (unsigned long long)memFrag.total_free
    );
    
    // Execute SQL
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}
