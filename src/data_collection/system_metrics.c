#include <stdio.h>
#include <pdh.h>
#include <psapi.h>
#include "system_metrics.h"
#include <winevt.h>    // For Windows Event Log
#include <windows.h>
#include <tlhelp32.h>    // For thread enumeration
#include <pdhmsg.h>
#include <time.h>
#include <wbemidl.h>
#include <limits.h>     // For ULLONG_MAX
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// Add the libraries we need to link against
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wevtapi.lib")  // For Event Log

// Global PDH (Performance Data Helper) variables
static PDH_HQUERY cpuQuery;
static PDH_HQUERY diskQuery;
static PDH_HCOUNTER cpuTotal;
static PDH_HCOUNTER diskRead;
static PDH_HCOUNTER diskWrite;

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
    SIZE_T total_free;           // Total free physical memory
    SIZE_T largest_free;         // Largest free block
    SIZE_T virtual_total;        // Total virtual memory
    SIZE_T virtual_total_free;   // Total free virtual memory
    DWORD free_block_count;      // Number of free memory blocks
    double avg_block_size;       // Average size of free blocks
    DWORD small_blocks;          // Blocks < 1MB
    DWORD medium_blocks;         // Blocks 1MB-16MB
    DWORD large_blocks;          // Blocks > 16MB
    double fragmentation_percent;
    double virtual_usage_percent; // Virtual memory usage percentage
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
    static double lastReadBytes = 0;
    static double lastWriteBytes = 0;
    static ULONGLONG lastCollectTime = 0;
    ULONGLONG currentTime = GetTickCount64();
    
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

    // Calculate time delta in seconds
    double timeDelta = (currentTime - lastCollectTime) / 1000.0;  // Convert to seconds
    
    if (lastCollectTime > 0 && timeDelta > 0) {
        // Calculate delta values and convert to bytes per second
        *read_bytes = (readVal.doubleValue - lastReadBytes) / timeDelta;
        *write_bytes = (writeVal.doubleValue - lastWriteBytes) / timeDelta;
        
        // Handle potential negative values due to counter reset
        if (*read_bytes < 0) *read_bytes = 0;
        if (*write_bytes < 0) *write_bytes = 0;
    } else {
        // First collection, initialize values
        *read_bytes = 0;
        *write_bytes = 0;
    }
    
    // Store values for next calculation
    lastReadBytes = readVal.doubleValue;
    lastWriteBytes = writeVal.doubleValue;
    lastCollectTime = currentTime;
    
    *queue_length = queueVal.longValue;
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
    double temperature = -1.0;  // Default to -1 if unable to get temperature
    HRESULT hr;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    
    // Initialize COM
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        log_error("get_cpu_temperature", "Failed to initialize COM");
        return temperature;
    }
    
    // Set general COM security levels
    hr = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );
    
    if (FAILED(hr)) {
        log_error("get_cpu_temperature", "Failed to initialize security");
        CoUninitialize();
        return temperature;
    }
    
    // Obtain the initial locator to WMI
    hr = CoCreateInstance(
        &CLSID_WbemLocator,           
        0, 
        CLSCTX_INPROC_SERVER, 
        &IID_IWbemLocator, 
        (LPVOID *) &pLoc
    );
    
    if (FAILED(hr)) {
        log_error("get_cpu_temperature", "Failed to create IWbemLocator object");
        CoUninitialize();
        return temperature;
    }
    
    // Connect to WMI through the IWbemLocator::ConnectServer method
    hr = pLoc->lpVtbl->ConnectServer(
        pLoc,
        L"ROOT\\WMI",                // Object path of WMI namespace
        NULL,                        // User name. NULL = current user
        NULL,                        // User password. NULL = current
        0,                          // Locale. NULL indicates current
        0,                          // Security flags    
        0,                          // Authority (e.g. Kerberos)
        0,                          // Context object 
        &pSvc                       // pointer to IWbemServices proxy
    );
    
    if (SUCCEEDED(hr)) {
        // Set security levels on the proxy
        hr = CoSetProxyBlanket(
            (IUnknown *)pSvc,           // Cast to IUnknown for COM interface
            RPC_C_AUTHN_WINNT,          // RPC_C_AUTHN_xxx
            RPC_C_AUTHZ_NONE,           // RPC_C_AUTHZ_xxx
            NULL,                        // Server principal name 
            RPC_C_AUTHN_LEVEL_CALL,     // RPC_C_AUTHN_LEVEL_xxx 
            RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
            NULL,                        // client identity
            EOAC_NONE                    // proxy capabilities 
        );
        
        if (SUCCEEDED(hr)) {
            IEnumWbemClassObject* pEnumerator = NULL;
            hr = pSvc->lpVtbl->ExecQuery(
                pSvc,
                L"WQL", 
                L"SELECT * FROM MSAcpi_ThermalZoneTemperature",
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
                NULL,
                &pEnumerator
            );
            
            if (SUCCEEDED(hr)) {
                IWbemClassObject *pclsObj = NULL;
                ULONG uReturn = 0;
                
                while (pEnumerator) {
                    hr = pEnumerator->lpVtbl->Next(pEnumerator, WBEM_INFINITE, 1, &pclsObj, &uReturn);
                    if (uReturn == 0) break;
                    
                    VARIANT vtProp;
                    hr = pclsObj->lpVtbl->Get(pclsObj, L"CurrentTemperature", 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hr)) {
                        // Convert temperature from deciKelvin to Celsius
                        temperature = (vtProp.intVal / 10.0) - 273.15;
                        VariantClear(&vtProp);
                    }
                    
                    pclsObj->lpVtbl->Release(pclsObj);
                    if (temperature != -1.0) break;  // Got valid temperature
                }
                
                pEnumerator->lpVtbl->Release(pEnumerator);
            }
        }
        pSvc->lpVtbl->Release(pSvc);
    }
    
    CoUninitialize();
    return temperature;
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
        // Add counters for disk latency and split I/O
        if (PdhAddCounterW(query, L"\\PhysicalDisk(_Total)\\Avg. Disk sec/Read", 0, &readTime) != ERROR_SUCCESS ||
            PdhAddCounterW(query, L"\\PhysicalDisk(_Total)\\Avg. Disk sec/Write", 0, &writeTime) != ERROR_SUCCESS ||
            PdhAddCounterW(query, L"\\PhysicalDisk(_Total)\\Split IO/Sec", 0, &splitIO) != ERROR_SUCCESS) {
            PdhCloseQuery(query);
            return info;
        }
        
        // First collection to establish baseline
        PdhCollectQueryData(query);
        Sleep(1000);  // Wait for a second sample
        
        if (PdhCollectQueryData(query) == ERROR_SUCCESS) {
            PDH_FMT_COUNTERVALUE value;
            
            // Get read latency (convert to milliseconds)
            if (PdhGetFormattedCounterValue(readTime, PDH_FMT_DOUBLE, NULL, &value) == ERROR_SUCCESS) {
                info.read_latency = value.doubleValue * 1000.0;  // Convert seconds to milliseconds
            }
            
            // Get write latency (convert to milliseconds)
            if (PdhGetFormattedCounterValue(writeTime, PDH_FMT_DOUBLE, NULL, &value) == ERROR_SUCCESS) {
                info.write_latency = value.doubleValue * 1000.0;  // Convert seconds to milliseconds
            }
            
            // Get split I/O count
            if (PdhGetFormattedCounterValue(splitIO, PDH_FMT_LONG, NULL, &value) == ERROR_SUCCESS) {
                info.split_io_count = value.longValue;
            }
        }
        
        PdhCloseQuery(query);
    }
    
    return info;
}

MemoryFragInfo get_memory_fragmentation(void) {
    MemoryFragInfo info = {0};
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&memInfo)) {
        // Get physical memory info with overflow check
        info.total_free = memInfo.ullAvailPhys;
        
        // Calculate virtual memory with overflow protection
        ULONGLONG totalPhysical = memInfo.ullTotalPhys;
        ULONGLONG totalPageFile = memInfo.ullTotalPageFile;
        ULONGLONG availPageFile = memInfo.ullAvailPageFile;
        
        // Check for potential overflow before addition
        if (totalPageFile > (ULLONG_MAX - totalPhysical)) {
            // Handle overflow case - use maximum safe value
            info.virtual_total = ULLONG_MAX;
        } else {
            info.virtual_total = totalPhysical + totalPageFile;
        }
        
        // Check for overflow in available memory calculation
        if (availPageFile > (ULLONG_MAX - memInfo.ullAvailPhys)) {
            // Handle overflow case - use maximum safe value
            info.virtual_total_free = ULLONG_MAX;
        } else {
            info.virtual_total_free = memInfo.ullAvailPhys + availPageFile;
        }
        
        // Ensure virtual_total_free doesn't exceed virtual_total
        if (info.virtual_total_free > info.virtual_total) {
            info.virtual_total_free = info.virtual_total;
        }
        
        // Calculate virtual memory usage percentage with proper casting
        if (info.virtual_total > 0) {
            double used = (double)(info.virtual_total - info.virtual_total_free);
            info.virtual_usage_percent = (used / (double)info.virtual_total) * 100.0;
            
            // Ensure percentage is within valid range
            if (info.virtual_usage_percent > 100.0) {
                info.virtual_usage_percent = 100.0;
            } else if (info.virtual_usage_percent < 0.0) {
                info.virtual_usage_percent = 0.0;
            }
        }
        
        // Get system info for memory ranges
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        // Get memory block information using VirtualQuery
        MEMORY_BASIC_INFORMATION memBasicInfo;
        LPVOID addr = sysInfo.lpMinimumApplicationAddress;
        SIZE_T totalVirtualFree = 0;
        SIZE_T totalBlockSize = 0;
        
        // Scan through the entire user-mode address space
        while (addr < sysInfo.lpMaximumApplicationAddress) {
            if (VirtualQuery(addr, &memBasicInfo, sizeof(memBasicInfo))) {
                if (memBasicInfo.State == MEM_FREE) {
                    // Track largest block (but only if it's a reasonable size)
                    if (memBasicInfo.RegionSize > info.largest_free && 
                        memBasicInfo.RegionSize <= info.total_free) {
                        info.largest_free = memBasicInfo.RegionSize;
                    }
                    
                    // Count blocks by size
                    if (memBasicInfo.RegionSize < (1024 * 1024)) { // < 1MB
                        info.small_blocks++;
                    } else if (memBasicInfo.RegionSize < (16 * 1024 * 1024)) { // 1MB-16MB
                        info.medium_blocks++;
                    } else if (memBasicInfo.RegionSize <= info.total_free) { // > 16MB but <= total free
                        info.large_blocks++;
                    }
                    
                    // Only count reasonable block sizes
                    if (memBasicInfo.RegionSize <= info.total_free) {
                        info.free_block_count++;
                        totalBlockSize += memBasicInfo.RegionSize;
                    }
                }
                
                // Move to next region
                addr = (LPVOID)((DWORD_PTR)memBasicInfo.BaseAddress + memBasicInfo.RegionSize);
                
                // Break if we wrap around or exceed max address
                if ((DWORD_PTR)addr < (DWORD_PTR)memBasicInfo.BaseAddress ||
                    addr >= sysInfo.lpMaximumApplicationAddress) {
                    break;
                }
            } else {
                // If VirtualQuery fails, move to the next page
                addr = (LPVOID)((DWORD_PTR)addr + sysInfo.dwPageSize);
            }
        }
        
        // Calculate average block size
        if (info.free_block_count > 0) {
            info.avg_block_size = (double)totalBlockSize / info.free_block_count;
        }
        
        // Calculate fragmentation based on physical memory
        if (info.total_free > 0) {
            // Use physical memory metrics for fragmentation calculation
            info.fragmentation_percent = ((double)(info.total_free - info.largest_free) / info.total_free) * 100.0;
            if (info.fragmentation_percent > 100.0) {
                info.fragmentation_percent = 100.0;
            } else if (info.fragmentation_percent < 0.0) {
                info.fragmentation_percent = 0.0;
            }
        }
    }
    
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
    double mem = get_memory_usage();
    
    // Get page file usage in bytes
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    double page_file = 0.0;
    if (GlobalMemoryStatusEx(&memInfo)) {
        page_file = (double)(memInfo.ullTotalPageFile - memInfo.ullAvailPageFile);
    }
    
    double disk_read, disk_write;
    int disk_queue;
    get_disk_metrics(&disk_read, &disk_write, &disk_queue);
    
    // Collect advanced metrics
    SystemEventCounts eventCounts = get_system_event_counts();
    DiskHealthInfo diskHealth = get_disk_health();
    MemoryFragInfo memFrag = get_memory_fragmentation();
    
    // Create SQL insert statement with all metrics
    snprintf(sql, sizeof(sql),
        "INSERT INTO system_metrics ("
        "timestamp, cpu_usage, memory_usage, "
        "page_file_usage, disk_read_bytes, disk_write_bytes, "
        "disk_queue_length, crash_count, error_count, warning_count, "
        "disk_read_latency, disk_write_latency, disk_split_io, "
        "memory_fragmentation, memory_largest_free, memory_total_free, "
        "memory_block_count, memory_avg_block_size, "
        "memory_small_blocks, memory_medium_blocks, memory_large_blocks, "
        "memory_virtual_total, memory_virtual_free, memory_virtual_usage"
        ") VALUES ("
        "'%s', %.2f, %.2f, %.2f, %.2f, %.2f, %d, "
        "%lu, %lu, %lu, %.2f, %.2f, %lu, %.2f, %llu, %llu, "
        "%lu, %.2f, %lu, %lu, %lu, %llu, %llu, %.2f"
        ");",
        timestamp, cpu, mem, page_file,
        disk_read, disk_write, disk_queue,
        eventCounts.crash_count, eventCounts.error_count, eventCounts.warning_count,
        diskHealth.read_latency, diskHealth.write_latency, diskHealth.split_io_count,
        memFrag.fragmentation_percent, 
        (unsigned long long)memFrag.largest_free, 
        (unsigned long long)memFrag.total_free,
        (unsigned long)memFrag.free_block_count,
        memFrag.avg_block_size,
        (unsigned long)memFrag.small_blocks,
        (unsigned long)memFrag.medium_blocks,
        (unsigned long)memFrag.large_blocks,
        (unsigned long long)memFrag.virtual_total,
        (unsigned long long)memFrag.virtual_total_free,
        memFrag.virtual_usage_percent
    );
    
    // Execute SQL
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}
