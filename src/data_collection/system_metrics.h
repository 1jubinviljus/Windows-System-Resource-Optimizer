#ifndef SYSTEM_METRICS_H
#define SYSTEM_METRICS_H

#include <windows.h>
#include <sqlite3.h>

// Main collection function that will be called from main.c
void collect_system_metrics(sqlite3 *db, const char *timestamp);

// Individual metric collection functions
double get_cpu_usage(void);
double get_cpu_temperature(void);
double get_memory_usage(void);
double get_page_file_usage(void);

// Disk metrics
void get_disk_metrics(double *read_bytes, double *write_bytes, int *queue_length);

// Network metrics
void get_network_metrics(double *bytes_in, double *bytes_out);

// System events/stability
void get_system_events(void);

#endif // SYSTEM_METRICS_H