#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Forward declarations
void displaySystemResources();
void displayMemoryUsage();
void displayCPUUsage();
void displayDiskUsage();

int main() {
    printf("System Optimizer is running!\n");

    //Call the function to display the system resource usage
    displaySystemResources();
    return 0;
}

// Function to display all system resource usage
void displaySystemResources(){
    //Display all the system resource usages
    displayMemoryUsage();
    displayCPUUsage();
    displayDiskUsage();
}

// Function to display memory usage
void displayMemoryUsage(){
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX); //set the struct size
    if(GlobalMemoryStatusEx(&memInfo)){ //True if memInfo is sucessfully filled
        printf("Memory Usage: %.2f%%\n", (float)(memInfo.dwMemoryLoad)); //% of memory usage
        printf("Total Physical Memory: %llu bytes\n", memInfo.ullTotalPhys); //total physical memory
        printf("Available Physical Memory: %llu bytes\n", memInfo.ullAvailPhys); //available physical memory
    } else { printf("Error retreiving memory usage"); }
}

// Function to display CPU usage
void displayCPUUsage() {
    SYSTEM_INFO sysInfo;
    FILETIME idleTime, kernelTime, userTime;
    
    GetSystemInfo(&sysInfo);
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        // For simplicity, we are just displaying system info as CPU usage
        printf("CPU Usage: Not Implemented (simplified version)\n");
    } else {
        printf("Error retrieving CPU usage\n");
    }
}

// Function to display disk usage
void displayDiskUsage() {
    ULARGE_INTEGER freeBytesToCaller, totalBytes, freeBytes;
    if (GetDiskFreeSpaceEx("C:\\", &freeBytesToCaller, &totalBytes, &freeBytes)) {
        printf("Disk Usage: %.2f%% used\n", 100.0 - ((float)freeBytes.QuadPart / (float)totalBytes.QuadPart) * 100);
    } else {
        printf("Error retrieving disk usage\n");
    }
}