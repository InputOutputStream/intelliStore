#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "../include/integrated_database.h"

static sqlite3* db = NULL;

// ==================== INITIALIZATION ====================

int db_init(const char* db_path) {
    int rc = sqlite3_open(db_path, &db);
    if (rc) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    // Enable foreign keys
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, 0);
    
    return 0;
}

void db_close() {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

// ==================== CLIENT MANAGEMENT ====================

int db_create_client(const char* name, int age, const char* sex, 
                     int fingerprint_id, const char* face_image_path) {
    const char* sql = 
        "INSERT INTO clients (name, age, sex, fingerprint_id, face_image_path) "
        "VALUES (?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, age);
    sqlite3_bind_text(stmt, 3, sex, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, fingerprint_id);
    sqlite3_bind_text(stmt, 5, face_image_path, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error creating client: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    return (int)sqlite3_last_insert_rowid(db);
}

int db_get_client_by_id(int client_id, Client* client) {
    if (!client) return -1;
    
    const char* sql = "SELECT * FROM clients WHERE client_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, client_id);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        client->client_id = sqlite3_column_int(stmt, 0);
        strncpy(client->name, (const char*)sqlite3_column_text(stmt, 1), 99);
        client->age = sqlite3_column_int(stmt, 2);
        strncpy(client->sex, (const char*)sqlite3_column_text(stmt, 3), 9);
        client->fingerprint_id = sqlite3_column_int(stmt, 4);
        strncpy(client->face_image_path, (const char*)sqlite3_column_text(stmt, 5), 254);
        client->is_active = sqlite3_column_int(stmt, 8);
        
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_get_client_by_fingerprint(int fingerprint_id, Client* client) {
    if (!client) return -1;
    
    const char* sql = "SELECT * FROM clients WHERE fingerprint_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, fingerprint_id);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        client->client_id = sqlite3_column_int(stmt, 0);
        strncpy(client->name, (const char*)sqlite3_column_text(stmt, 1), 99);
        client->age = sqlite3_column_int(stmt, 2);
        strncpy(client->sex, (const char*)sqlite3_column_text(stmt, 3), 9);
        client->fingerprint_id = sqlite3_column_int(stmt, 4);
        strncpy(client->face_image_path, (const char*)sqlite3_column_text(stmt, 5), 254);
        client->is_active = sqlite3_column_int(stmt, 8);
        
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_get_all_clients(Client** clients, int* count) {
    if (!clients || !count) return -1;
    
    const char* sql = "SELECT * FROM clients WHERE is_active = 1 ORDER BY name";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    // Count rows
    int num_rows = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) num_rows++;
    sqlite3_reset(stmt);
    
    if (num_rows == 0) {
        *clients = NULL;
        *count = 0;
        sqlite3_finalize(stmt);
        return 0;
    }
    
    // Allocate memory
    *clients = (Client*)malloc(num_rows * sizeof(Client));
    if (!*clients) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    // Read data
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < num_rows) {
        (*clients)[i].client_id = sqlite3_column_int(stmt, 0);
        strncpy((*clients)[i].name, (const char*)sqlite3_column_text(stmt, 1), 99);
        (*clients)[i].age = sqlite3_column_int(stmt, 2);
        strncpy((*clients)[i].sex, (const char*)sqlite3_column_text(stmt, 3), 9);
        (*clients)[i].fingerprint_id = sqlite3_column_int(stmt, 4);
        strncpy((*clients)[i].face_image_path, (const char*)sqlite3_column_text(stmt, 5), 254);
        (*clients)[i].is_active = sqlite3_column_int(stmt, 8);
        i++;
    }
    
    *count = i;
    sqlite3_finalize(stmt);
    return 0;
}

// ==================== PRODUCT MANAGEMENT ====================

int db_create_product(const char* name, const char* type, double price,
                     const char* expiry_date, const char* image_path,
                     int visual_signature_id, int stock_quantity) {
    const char* sql = 
        "INSERT INTO products (name, type, price, expiry_date, image_path, "
        "visual_signature_id, stock_quantity) VALUES (?, ?, ?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, type, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, price);
    
    if (expiry_date && strlen(expiry_date) > 0) {
        sqlite3_bind_text(stmt, 4, expiry_date, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 4);
    }
    
    sqlite3_bind_text(stmt, 5, image_path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, visual_signature_id);
    sqlite3_bind_int(stmt, 7, stock_quantity);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return -1;
    }
    
    return (int)sqlite3_last_insert_rowid(db);
}

int db_get_product_by_visual_id(int visual_signature_id, Product* product) {
    if (!product) return -1;
    
    const char* sql = "SELECT * FROM products WHERE visual_signature_id = ? AND is_active = 1";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, visual_signature_id);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        product->product_id = sqlite3_column_int(stmt, 0);
        strncpy(product->name, (const char*)sqlite3_column_text(stmt, 1), 99);
        strncpy(product->type, (const char*)sqlite3_column_text(stmt, 2), 49);
        product->price = sqlite3_column_double(stmt, 3);
        
        const char* exp = (const char*)sqlite3_column_text(stmt, 4);
        if (exp) strncpy(product->expiry_date, exp, 19);
        else product->expiry_date[0] = '\0';
        
        strncpy(product->image_path, (const char*)sqlite3_column_text(stmt, 5), 254);
        product->visual_signature_id = sqlite3_column_int(stmt, 6);
        product->stock_quantity = sqlite3_column_int(stmt, 7);
        product->is_active = sqlite3_column_int(stmt, 10);
        
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

// ==================== SHOPPING SESSION MANAGEMENT ====================

int db_create_session(int client_id) {
    const char* sql = "INSERT INTO shopping_sessions (client_id) VALUES (?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, client_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return -1;
    }
    
    return (int)sqlite3_last_insert_rowid(db);
}

int db_get_active_session(int client_id, ShoppingSession* session) {
    if (!session) return -1;
    
    const char* sql = 
        "SELECT * FROM shopping_sessions "
        "WHERE client_id = ? AND status = 'active' "
        "ORDER BY entry_time DESC LIMIT 1";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, client_id);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        session->session_id = sqlite3_column_int(stmt, 0);
        session->client_id = sqlite3_column_int(stmt, 1);
        strncpy(session->entry_time, (const char*)sqlite3_column_text(stmt, 2), 29);
        
        const char* exit = (const char*)sqlite3_column_text(stmt, 3);
        if (exit) strncpy(session->exit_time, exit, 29);
        else session->exit_time[0] = '\0';
        
        strncpy(session->status, (const char*)sqlite3_column_text(stmt, 4), 19);
        session->face_verified = sqlite3_column_int(stmt, 5);
        session->fingerprint_verified = sqlite3_column_int(stmt, 6);
        
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_verify_face_in_session(int session_id, bool verified) {
    const char* sql = "UPDATE shopping_sessions SET face_verified = ? WHERE session_id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, verified ? 1 : 0);
    sqlite3_bind_int(stmt, 2, session_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_verify_fingerprint_in_session(int session_id, bool verified) {
    const char* sql = "UPDATE shopping_sessions SET fingerprint_verified = ? WHERE session_id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, verified ? 1 : 0);
    sqlite3_bind_int(stmt, 2, session_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_close_session(int session_id) {
    const char* sql = 
        "UPDATE shopping_sessions SET status = 'completed', exit_time = CURRENT_TIMESTAMP "
        "WHERE session_id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, session_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== TRANSACTION MANAGEMENT ====================

int db_create_transaction(int session_id, int client_id, double total_amount,
                          const char* payment_method) {
    const char* sql = 
        "INSERT INTO transactions (session_id, client_id, total_amount, payment_method) "
        "VALUES (?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, session_id);
    sqlite3_bind_int(stmt, 2, client_id);
    sqlite3_bind_double(stmt, 3, total_amount);
    sqlite3_bind_text(stmt, 4, payment_method, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return -1;
    }
    
    return (int)sqlite3_last_insert_rowid(db);
}

int db_add_transaction_item(int transaction_id, int product_id, 
                            int quantity, double unit_price) {
    const char* sql = 
        "INSERT INTO transaction_items (transaction_id, product_id, quantity, unit_price, subtotal) "
        "VALUES (?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    double subtotal = quantity * unit_price;
    
    sqlite3_bind_int(stmt, 1, transaction_id);
    sqlite3_bind_int(stmt, 2, product_id);
    sqlite3_bind_int(stmt, 3, quantity);
    sqlite3_bind_double(stmt, 4, unit_price);
    sqlite3_bind_double(stmt, 5, subtotal);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_complete_transaction(int transaction_id, const char* invoice_path) {
    const char* sql = 
        "UPDATE transactions SET payment_status = 'completed', invoice_path = ? "
        "WHERE transaction_id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, invoice_path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, transaction_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== LOGGING ====================

int db_log_fingerprint_action(int client_id, int fingerprint_id, 
                              const char* action_type, bool success) {
    const char* sql = 
        "INSERT INTO fingerprint_logs (client_id, fingerprint_id, action_type, success) "
        "VALUES (?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, client_id);
    sqlite3_bind_int(stmt, 2, fingerprint_id);
    sqlite3_bind_text(stmt, 3, action_type, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, success ? 1 : 0);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}