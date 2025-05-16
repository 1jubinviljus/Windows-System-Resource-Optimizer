#include <stdio.h>
#include "sqlite3.h"

int main2() {
    sqlite3 *db;
    char *err_msg = 0;

    // Open database (creates it if it doesn't exist)
    int rc = sqlite3_open("build/optimizer.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Create table
    const char *sql = "CREATE TABLE IF NOT EXISTS stats(id INTEGER PRIMARY KEY, name TEXT);"
                      "INSERT INTO stats(name) VALUES('Test Run');";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("Table created and row inserted.\n");
    }

    sqlite3_close(db);
    return 0;
}
