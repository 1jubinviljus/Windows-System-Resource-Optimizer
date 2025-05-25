#ifndef SYSTEM_METRICS_H
#define SYSTEM_METRICS_H

#include <windows.h>
#include <sqlite3.h>

struct SystemMetrics {
    double cpu_usage;
    double memory_usage;
    double disk_usage;
};

// Main function that will collect metrics
void collect_system_metrics(sqlite3 *db, const char *timestamp);

#endif