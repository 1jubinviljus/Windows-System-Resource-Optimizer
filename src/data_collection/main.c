#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <sqlite3.h>
#include "system_metrics.h"
#include "process_metrics.h"
#include "../resource_management/process_optimizer.h"

// Global flag for graceful shutdown
static volatile bool running = true;

// Signal handler for graceful termination (Ctrl+C)
void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\nReceived termination signal. Cleaning up...\n");
        running = false;
    }
}

// Initialize database tables
static bool initialize_database(sqlite3 *db) {
    char *err_msg = NULL;
    FILE *fp = fopen("src/data_collection/init_db.sql", "r");
    if (!fp) {
        fprintf(stderr, "Failed to open init_db.sql\n");
        return false;
    }

    // Read the entire SQL file
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *sql = malloc(fsize + 1);
    if (!sql) {
        fclose(fp);
        fprintf(stderr, "Failed to allocate memory for SQL\n");
        return false;
    }

    fread(sql, fsize, 1, fp);
    sql[fsize] = 0;
    fclose(fp);

    // Execute the SQL
    if (sqlite3_exec(db, sql, NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        free(sql);
        return false;
    }

    free(sql);
    return true;
}

int main(int argc, char *argv[]) {
    // Default collection interval (in milliseconds)
    const DWORD collection_interval = 5000;  // 5 seconds
    const DWORD optimization_interval = 30000; // 30 seconds
    sqlite3 *db = NULL;
    time_t now;
    char timestamp[64];
    DWORD last_optimization = 0;
    
    // Set up signal handling for graceful termination
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize SQLite database
    if (sqlite3_open("system_metrics.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Initialize database tables
    if (!initialize_database(db)) {
        sqlite3_close(db);
        return 1;
    }
    
    printf("Starting system metrics collection and optimization (Press Ctrl+C to stop)...\n");
    printf("Collection interval: %lu ms\n", collection_interval);
    printf("Optimization interval: %lu ms\n", optimization_interval);
    
    // Main collection loop
    while (running) {
        // Get current timestamp
        time(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        // Collect and store metrics
        collect_system_metrics(db, timestamp);
        collect_process_metrics(db, timestamp);
        
        // Run optimization periodically
        DWORD current_time = GetTickCount();
        if (current_time - last_optimization >= optimization_interval) {
            printf("Running process optimization...\n");
            if (optimize_engineering_processes()) {
                printf("Engineering processes optimized successfully\n");
            } else {
                printf("No engineering processes found or optimization failed\n");
            }
            last_optimization = current_time;
        }
        
        // Print a status message
        printf("Metrics collected at %s\n", timestamp);
        
        // Sleep for the specified interval
        Sleep(collection_interval);
    }
    
    // Cleanup
    printf("Closing database connection...\n");
    sqlite3_close(db);
    printf("Collection stopped.\n");
    
    return 0;
}
