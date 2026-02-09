#ifndef INTEGRATED_DATABASE_H
#define INTEGRATED_DATABASE_H

#include <stdlib.h>
#include <stdbool.h>

// ==================== STRUCTURES ====================

typedef struct {
    int client_id;
    char name[100];
    int age;
    char sex[10];
    int fingerprint_id;
    char face_image_path[255];
    bool is_active;
} Client;

typedef struct {
    int product_id;
    char name[100];
    char type[50];
    double price;
    char expiry_date[20];
    char image_path[255];
    int visual_signature_id;
    int stock_quantity;
    bool is_active;
} Product;

typedef struct {
    int session_id;
    int client_id;
    char entry_time[30];
    char exit_time[30];
    char status[20];
    bool face_verified;
    bool fingerprint_verified;
} ShoppingSession;

typedef struct {
    int transaction_id;
    int session_id;
    int client_id;
    double total_amount;
    char payment_status[20];
    char payment_method[20];
    char invoice_path[255];
    char transaction_time[30];
} Transaction;

typedef struct {
    int item_id;
    int transaction_id;
    int product_id;
    int quantity;
    double unit_price;
    double subtotal;
} TransactionItem;

// ==================== DATABASE INITIALIZATION ====================

int db_init(const char* db_path);
void db_close();

// ==================== CLIENT MANAGEMENT ====================

int db_create_client(const char* name, int age, const char* sex, 
                     int fingerprint_id, const char* face_image_path);
int db_get_client_by_id(int client_id, Client* client);
int db_get_client_by_fingerprint(int fingerprint_id, Client* client);
int db_get_client_by_face(const char* face_image_path, Client* client);
int db_get_all_clients(Client** clients, int* count);
int db_update_client(int client_id, const char* name, int age, const char* sex);
int db_delete_client(int client_id);

// ==================== PRODUCT MANAGEMENT ====================

int db_create_product(const char* name, const char* type, double price,
                     const char* expiry_date, const char* image_path,
                     int visual_signature_id, int stock_quantity);
int db_get_product_by_id(int product_id, Product* product);
int db_get_product_by_visual_id(int visual_signature_id, Product* product);
int db_get_all_products(Product** products, int* count);
int db_update_product_stock(int product_id, int quantity);
int db_delete_product(int product_id);

// ==================== SHOPPING SESSION MANAGEMENT ====================

int db_create_session(int client_id);
int db_get_active_session(int client_id, ShoppingSession* session);
int db_verify_face_in_session(int session_id, bool verified);
int db_verify_fingerprint_in_session(int session_id, bool verified);
int db_close_session(int session_id);
int db_abandon_session(int session_id);

// ==================== TRANSACTION MANAGEMENT ====================

int db_create_transaction(int session_id, int client_id, double total_amount,
                          const char* payment_method);
int db_add_transaction_item(int transaction_id, int product_id, 
                            int quantity, double unit_price);
int db_complete_transaction(int transaction_id, const char* invoice_path);
int db_fail_transaction(int transaction_id);
int db_get_transaction_items(int transaction_id, TransactionItem** items, int* count);

// ==================== LOGGING & AUDIT ====================

int db_log_fingerprint_action(int client_id, int fingerprint_id, 
                              const char* action_type, bool success);
int db_get_recent_logs(int limit, char*** logs, int* count);

// ==================== STATISTICS ====================

int db_get_client_stats(int* total_clients, int* active_sessions, 
                       int* total_transactions);
int db_get_product_stats(int* total_products, int* low_stock_count);
int db_get_revenue_today(double* revenue);

#endif // INTEGRATED_DATABASE_H