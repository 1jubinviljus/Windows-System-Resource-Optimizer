#include <stdio.h>
#include <sqlite3.h>

static int callback(void *data, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int main() {
    sqlite3 *db;
    char *err_msg = 0;
    
    if (sqlite3_open("system_metrics.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    const char *sql = "SELECT timestamp, cpu_usage, memory_usage, "
                     "disk_read_bytes, disk_write_bytes, "
                     "network_bytes_in, network_bytes_out "
                     "FROM system_metrics ORDER BY timestamp DESC LIMIT 5;";
    
    printf("Last 5 system metrics:\n\n");
    
    if (sqlite3_exec(db, sql, callback, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    sqlite3_close(db);
    return 0;
} 