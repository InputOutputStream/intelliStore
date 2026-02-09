#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "../include/database.h"
#include "../external/sqlite3.h"

static sqlite3* db = NULL;

int db_init(const char* db_path) {
    int rc = sqlite3_open(db_path, &db);
    if (rc) {
        fprintf(stderr, "Erreur ouverture DB: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    // Creer les tables si elles n'existent pas
    char* sql = 
        "CREATE TABLE IF NOT EXISTS users ("
        "   user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   name TEXT NOT NULL,"
        "   fingerprint_id INTEGER UNIQUE NOT NULL,"
        "   created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");"
        
        "CREATE TABLE IF NOT EXISTS logs ("
        "   log_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   user_id INTEGER NOT NULL,"
        "   timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "   FOREIGN KEY (user_id) REFERENCES users(user_id)"
        ");"
        
        "CREATE INDEX IF NOT EXISTS idx_fingerprint_id ON users(fingerprint_id);"
        "CREATE INDEX IF NOT EXISTS idx_logs_user ON logs(user_id);"
        "CREATE INDEX IF NOT EXISTS idx_logs_time ON logs(timestamp);";
    
    char* err_msg = NULL;
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erreur creation tables: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        db = NULL;
        return -1;
    }
    
    return 0;
}

int db_create_user(const char* name, int fingerprint_id) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT INTO users (name, fingerprint_id) VALUES (?, ?)");
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, fingerprint_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return -1;
    }
    
    return (int)sqlite3_last_insert_rowid(db);
}

int db_get_user_by_fingerprint(int fingerprint_id, User* user) {
    if (!user) return -1;
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM users WHERE fingerprint_id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, fingerprint_id);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user->user_id = sqlite3_column_int(stmt, 0);
        strncpy(user->name, (const char*)sqlite3_column_text(stmt, 1), sizeof(user->name)-1);
        user->fingerprint_id = sqlite3_column_int(stmt, 2);
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_get_user_by_id(int user_id, User* user) {
    if (!user) return -1;
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM users WHERE user_id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user->user_id = sqlite3_column_int(stmt, 0);
        strncpy(user->name, (const char*)sqlite3_column_text(stmt, 1), sizeof(user->name)-1);
        user->fingerprint_id = sqlite3_column_int(stmt, 2);
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_get_all_users(User** users, int* count) {
    if (!users || !count) return -1;
    
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM users ORDER BY name";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    // Compter d'abord
    int num_rows = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        num_rows++;
    }
    sqlite3_reset(stmt);
    
    if (num_rows == 0) {
        *users = NULL;
        *count = 0;
        sqlite3_finalize(stmt);
        return 0;
    }
    
    // Allouer memoire
    *users = (User*)malloc(num_rows * sizeof(User));
    if (!*users) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    // Lire les donnees
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < num_rows) {
        (*users)[i].user_id = sqlite3_column_int(stmt, 0);
        const char* name = (const char*)sqlite3_column_text(stmt, 1);
        strncpy((*users)[i].name, name ? name : "", sizeof((*users)[i].name)-1);
        (*users)[i].fingerprint_id = sqlite3_column_int(stmt, 2);
        i++;
    }
    
    *count = i;
    sqlite3_finalize(stmt);
    return 0;
}

int db_delete_user(int user_id) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "DELETE FROM users WHERE user_id = ?");
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_log_entry(int user_id) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT INTO logs (user_id) VALUES (?)");
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_recent_logs(int limit, char*** logs, int* count) {
    if (!logs || !count) return -1;
    
    sqlite3_stmt* stmt;
    const char* sql = 
        "SELECT u.name, l.timestamp "
        "FROM logs l "
        "JOIN users u ON l.user_id = u.user_id "
        "ORDER BY l.timestamp DESC "
        "LIMIT ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, limit);
    
    // Allouer memoire
    *logs = (char**)malloc(limit * sizeof(char*));
    if (!*logs) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    // Lire les logs
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < limit) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        const char* timestamp = (const char*)sqlite3_column_text(stmt, 1);
        
        // Formater la chaine
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%-20s - %s", 
                name ? name : "Inconnu", 
                timestamp ? timestamp : "N/A");
        
        (*logs)[i] = strdup(buffer);
        i++;
    }
    
    *count = i;
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_stats(int* total_users, int* total_logs, int* today_logs) {
    if (!total_users || !total_logs || !today_logs) return -1;
    
    // Total utilisateurs
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM users";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_step(stmt);
    *total_users = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    // Total logs
    sql = "SELECT COUNT(*) FROM logs";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_step(stmt);
    *total_logs = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    // Logs aujourd'hui
    sql = "SELECT COUNT(*) FROM logs WHERE DATE(timestamp) = DATE('now')";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_step(stmt);
    *today_logs = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    return 0;
}

void db_close() {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}