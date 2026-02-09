#ifndef DATABASE_H
#define DATABASE_H

#include "fingerprint.h"
#include <stdlib.h>

// Initialisation base de données
int db_init(const char* db_path);

// Gestion des utilisateurs
int db_create_user(const char* name, int fingerprint_id);
int db_get_user_by_fingerprint(int fingerprint_id, User* user);
int db_get_user_by_id(int user_id, User* user);
int db_get_all_users(User** users, int* count);
int db_delete_user(int user_id);

// Gestion des logs
int db_log_entry(int user_id);
int db_get_recent_logs(int limit, char*** logs, int* count);
int db_get_stats(int* total_users, int* total_logs, int* today_logs);

// Nettoyage
void db_close();

#endif