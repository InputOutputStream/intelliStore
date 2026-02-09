#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/integrated_database.h"
#include "include/fingerprint.h"

#define DB_PATH "database/smart_store.db"
#define SERIAL_PORT "/dev/ttyACM0"
#define API_NOTIFICATION_URL "http://localhost:5000/api/register"

// Forward declarations
void clear_screen();
void wait_enter();
void print_header(const char* title);
int capture_face_image(int id, char* output_path);
int capture_product_image(int id, char* output_path);
void send_registration_to_api(int client_id, const char* type);
void list_products();
void list_all_products();

// ==================== MENU FUNCTIONS ====================

void print_main_menu() {
    clear_screen();
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║       SMART STORE - REGISTRATION SYSTEM              ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n\n");
    printf("  REGISTRATION:\n");
    printf("  1. Register New Client (Person)\n");
    printf("  2. Register New Product\n\n");
    printf("  MANAGEMENT:\n");
    printf("  3. List All Clients\n");
    printf("  4. List All Products\n");
    printf("  5. Test Fingerprint Sensor\n\n");
    printf("  SYSTEM:\n");
    printf("  6. View System Statistics\n");
    printf("  7. Exit\n\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("Choice: ");
}

// ==================== CLIENT REGISTRATION ====================

void register_client() {
    char name[100];
    int age;
    char sex[10];
    char face_image_path[255];
    
    print_header("CLIENT REGISTRATION");
    
    // Collect personal information
    printf("┌─── Personal Information ───────────────────────────┐\n\n");
    
    printf("Full Name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    
    if (strlen(name) == 0) {
        printf("\n✗ Invalid name\n");
        wait_enter();
        return;
    }
    
    printf("Age: ");
    if (scanf("%d", &age) != 1 || age < 0 || age > 150) {
        printf("\n✗ Invalid age\n");
        while (getchar() != '\n');
        wait_enter();
        return;
    }
    while (getchar() != '\n'); // Clear buffer
    
    printf("Sex (M/F/Other): ");
    fgets(sex, sizeof(sex), stdin);
    sex[strcspn(sex, "\n")] = '\0';
    
    if (strlen(sex) == 0) {
        strcpy(sex, "Other");
    }
    
    printf("\n└────────────────────────────────────────────────────┘\n\n");
    
    // Fingerprint enrollment
    printf("┌─── Fingerprint Enrollment ─────────────────────────┐\n\n");
    printf("Preparing fingerprint sensor...\n\n");
    
    int fingerprint_id = fingerprint_enroll(name);
    
    if (fingerprint_id <= 0) {
        printf("\n✗ FINGERPRINT ENROLLMENT FAILED\n");
        wait_enter();
        return;
    }
    
    printf("\n✓ Fingerprint registered with ID: %d\n", fingerprint_id);
    printf("\n└────────────────────────────────────────────────────┘\n\n");
    
    // Face capture
    printf("┌─── Face Image Capture ─────────────────────────────┐\n\n");
    printf("Please look at the camera...\n");
    printf("Capturing in 3 seconds...\n");
    sleep(3);
    
    // Call Python script to capture face
    int temp_id = get_next_available_id();
    if (capture_face_image(temp_id, face_image_path) != 0) {
        printf("\n✗ FACE CAPTURE FAILED\n");
        printf("Using default path...\n");
        snprintf(face_image_path, sizeof(face_image_path), 
                "../images/clients/%d.jpg", fingerprint_id);
    }
    
    printf("✓ Face image saved: %s\n", face_image_path);
    printf("\n└────────────────────────────────────────────────────┘\n\n");
    
    // Save to database
    int client_id = db_create_client(name, age, sex, fingerprint_id, face_image_path);
    
    if (client_id > 0) {
        printf("╔════════════════════════════════════════════════════╗\n");
        printf("║          CLIENT REGISTERED SUCCESSFULLY           ║\n");
        printf("╚════════════════════════════════════════════════════╝\n\n");
        printf("  Client ID:      %d\n", client_id);
        printf("  Name:           %s\n", name);
        printf("  Age:            %d\n", age);
        printf("  Sex:            %s\n", sex);
        printf("  Fingerprint ID: %d\n", fingerprint_id);
        printf("  Face Image:     %s\n", face_image_path);
        printf("\n");
        
        // Log the action
        db_log_fingerprint_action(client_id, fingerprint_id, "entry", true);
        
        // Notify API
        send_registration_to_api(client_id, "client");
        
    } else {
        printf("\n✗ DATABASE ERROR: Failed to save client\n");
    }
    
    wait_enter();
}

// ==================== PRODUCT REGISTRATION ====================

void register_product() {
    char name[100];
    char type[50];
    double price;
    char expiry_date[20];
    int stock_quantity;
    char image_path[255];
    char has_expiry;
    
    print_header("PRODUCT REGISTRATION");
    
    printf("┌─── Product Information ────────────────────────────┐\n\n");
    
    printf("Product Name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    
    if (strlen(name) == 0) {
        printf("\n✗ Invalid product name\n");
        wait_enter();
        return;
    }
    
    printf("Product Type (e.g., Electronics, Food, Accessories): ");
    fgets(type, sizeof(type), stdin);
    type[strcspn(type, "\n")] = '\0';
    
    if (strlen(type) == 0) {
        strcpy(type, "General");
    }
    
    printf("Price (EUR): ");
    if (scanf("%lf", &price) != 1 || price < 0) {
        printf("\n✗ Invalid price\n");
        while (getchar() != '\n');
        wait_enter();
        return;
    }
    while (getchar() != '\n');
    
    printf("Has expiry date? (y/n): ");
    scanf("%c", &has_expiry);
    while (getchar() != '\n');
    
    if (has_expiry == 'y' || has_expiry == 'Y') {
        printf("Expiry Date (YYYY-MM-DD): ");
        fgets(expiry_date, sizeof(expiry_date), stdin);
        expiry_date[strcspn(expiry_date, "\n")] = '\0';
    } else {
        expiry_date[0] = '\0';
    }
    
    printf("Initial Stock Quantity: ");
    if (scanf("%d", &stock_quantity) != 1 || stock_quantity < 0) {
        printf("\n✗ Invalid quantity\n");
        while (getchar() != '\n');
        wait_enter();
        return;
    }
    while (getchar() != '\n');
    
    printf("\n└────────────────────────────────────────────────────┘\n\n");
    
    // Visual signature ID (auto-generate or manual)
    int visual_signature_id;
    printf("┌─── Visual Recognition Setup ───────────────────────┐\n\n");
    printf("Visual Signature ID (or 0 for auto): ");
    if (scanf("%d", &visual_signature_id) != 1) {
        visual_signature_id = 0;
    }
    while (getchar() != '\n');
    
    if (visual_signature_id == 0) {
        // Auto-generate based on next available
        visual_signature_id = 1000 + (rand() % 9000);
        printf("Auto-generated Visual ID: %d\n", visual_signature_id);
    }
    
    printf("\n└────────────────────────────────────────────────────┘\n\n");
    
    // Image capture
    printf("┌─── Product Image Capture ──────────────────────────┐\n\n");
    printf("Position the product in front of camera...\n");
    printf("Capturing in 3 seconds...\n");
    sleep(3);
    
    if (capture_product_image(visual_signature_id, image_path) != 0) {
        printf("\n✗ IMAGE CAPTURE FAILED\n");
        printf("Using default path...\n");
        snprintf(image_path, sizeof(image_path), 
                "../images/produits/%d.jpg", visual_signature_id);
    }
    
    printf("✓ Product image saved: %s\n", image_path);
    printf("\n└────────────────────────────────────────────────────┘\n\n");
    
    // Save to database
    int product_id = db_create_product(name, type, price, 
                                       strlen(expiry_date) > 0 ? expiry_date : NULL,
                                       image_path, visual_signature_id, stock_quantity);
    
    if (product_id > 0) {
        printf("╔════════════════════════════════════════════════════╗\n");
        printf("║         PRODUCT REGISTERED SUCCESSFULLY           ║\n");
        printf("╚════════════════════════════════════════════════════╝\n\n");
        printf("  Product ID:     %d\n", product_id);
        printf("  Name:           %s\n", name);
        printf("  Type:           %s\n", type);
        printf("  Price:          %.2f EUR\n", price);
        printf("  Visual ID:      %d\n", visual_signature_id);
        printf("  Stock:          %d\n", stock_quantity);
        if (strlen(expiry_date) > 0) {
            printf("  Expiry Date:    %s\n", expiry_date);
        }
        printf("  Image:          %s\n", image_path);
        printf("\n");
        
        // Notify API
        send_registration_to_api(product_id, "product");
        
    } else {
        printf("\n✗ DATABASE ERROR: Failed to save product\n");
    }
    
    wait_enter();
}

// ==================== LIST FUNCTIONS ====================

void list_clients() {
    print_header("REGISTERED CLIENTS");
    
    Client* clients = NULL;
    int count = 0;
    
    if (db_get_all_clients(&clients, &count) == 0 && count > 0) {
        printf("Total Clients: %d\n\n", count);
        printf("┌─────┬──────────────────────┬─────┬────────┬──────────────┐\n");
        printf("│ ID  │ Name                 │ Age │ Sex    │ Fingerprint  │\n");
        printf("├─────┼──────────────────────┼─────┼────────┼──────────────┤\n");
        
        for (int i = 0; i < count; i++) {
            printf("│ %-3d │ %-20.20s │ %-3d │ %-6s │ %-12d │\n",
                   clients[i].client_id,
                   clients[i].name,
                   clients[i].age,
                   clients[i].sex,
                   clients[i].fingerprint_id);
        }
        
        printf("└─────┴──────────────────────┴─────┴────────┴──────────────┘\n");
        
        free(clients);
    } else {
        printf("No clients registered.\n");
    }
    
    wait_enter();
}

void list_products() {
    print_header("REGISTERED PRODUCTS");
    
    Product* products = NULL;
    int count = 0;
    
    if (db_get_all_products(&products, &count) == 0 && count > 0) {
        printf("Total Products: %d\n\n", count);
        printf("┌─────┬──────────────────────┬──────────┬─────────┬──────────┐\n");
        printf("│ ID  │ Name                 │ Type     │ Price   │ Stock    │\n");
        printf("├─────┼──────────────────────┼──────────┼─────────┼──────────┤\n");
        
        for (int i = 0; i < count; i++) {
            printf("│ %-3d │ %-20.20s │ %-8.8s │ %7.2f │ %-8d │\n",
                   products[i].product_id,
                   products[i].name,
                   products[i].type,
                   products[i].price,
                   products[i].stock_quantity);
        }
        
        printf("└─────┴──────────────────────┴──────────┴─────────┴──────────┘\n");
        
        free(products);
    } else {
        printf("No products registered.\n");
    }
    
    wait_enter();
}

void list_all_products() {
    Product* products = NULL;
    int count = 0;
    
    printf("═══════════════════════════════════════════════════════\n");
    printf("              PRODUCT INVENTORY LIST\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    // Retrieve all active products from database
    if (db_get_all_products(&products, &count) != 0) {
        printf("❌ Error retrieving products from database\n");
        return;
    }
    
    // Check if any products exist
    if (count == 0) {
        printf("ℹ️  No products found in the database\n");
        printf("   Use the registration system to add products.\n");
        return;
    }
    
    // Display products in a formatted table
    printf("Found %d product(s):\n\n", count);
    printf("%-4s %-25s %-15s %12s %8s %15s\n", 
           "ID", "Name", "Type", "Price", "Stock", "Visual ID");
    printf("───────────────────────────────────────────────────────────────────────────────\n");
    
    for (int i = 0; i < count; i++) {
        printf("%-4d %-25s %-15s %9.2f XAF %8d %15d\n",
               products[i].product_id,
               products[i].name,
               products[i].type,
               products[i].price,
               products[i].stock_quantity,
               products[i].visual_signature_id);
        
        // Show expiry date if available
        if (products[i].expiry_date[0] != '\0') {
            printf("     ⏰ Expires: %s\n", products[i].expiry_date);
        }
        
        // Warn about low stock
        if (products[i].stock_quantity < 10) {
            printf("     ⚠️  LOW STOCK WARNING\n");
        }
        
        printf("\n");
    }
    
    // Calculate total inventory value
    double total_value = 0.0;
    int total_items = 0;
    
    for (int i = 0; i < count; i++) {
        total_value += products[i].price * products[i].stock_quantity;
        total_items += products[i].stock_quantity;
    }
    
    printf("───────────────────────────────────────────────────────────────────────────────\n");
    printf("SUMMARY:\n");
    printf("  Total Products: %d\n", count);
    printf("  Total Items in Stock: %d\n", total_items);
    printf("  Total Inventory Value: %.2f XAF\n", total_value);
    printf("═══════════════════════════════════════════════════════\n");
    
    // IMPORTANT: Free the allocated memory
    free(products);
}

// ==================== HELPER FUNCTIONS ====================

void clear_screen() {
    printf("\033[2J\033[H");
}

void wait_enter() {
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

void print_header(const char* title) {
    clear_screen();
    printf("═══════════════════════════════════════════════════════\n");
    printf("  %s\n", title);
    printf("═══════════════════════════════════════════════════════\n\n");
}

int capture_face_image(int id, char* output_path) {
    char cmd[512];
    snprintf(output_path, 255, "../images/clients/%d.jpg", id);
    
    // Call Python script to capture from camera
    snprintf(cmd, sizeof(cmd), 
            "python3 ../python/capture_face.py %d %s", id, output_path);
    
    return system(cmd);
}

int capture_product_image(int id, char* output_path) {
    char cmd[512];
    snprintf(output_path, 255, "../images/produits/%d.jpg", id);
    
    // Call Python script to capture from camera
    snprintf(cmd, sizeof(cmd), 
            "python3 ../python/capture_product.py %d %s", id, output_path);
    
    return system(cmd);
}

void send_registration_to_api(int id, const char* type) {
    // Simple notification - could be enhanced with libcurl
    printf("→ Notifying API system...\n");
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
            "curl -X POST http://localhost:5000/api/notify "
            "-H 'Content-Type: application/json' "
            "-d '{\"type\":\"%s\",\"id\":%d}' 2>/dev/null",
            type, id);
    
    system(cmd);
}

// ==================== MAIN PROGRAM ====================

int main() {
    printf("Initializing Smart Store Registration System...\n");
    
    // Initialize database
    if (db_init(DB_PATH) != 0) {
        printf("✗ Database initialization failed\n");
        return 1;
    }
    printf("✓ Database initialized\n");
    
    // Initialize fingerprint sensor
    if (fingerprint_init(SERIAL_PORT) != 0) {
        printf("✗ Fingerprint sensor initialization failed\n");
        printf("  Check port: %s\n", SERIAL_PORT);
        db_close();
        return 1;
    }
    printf("✓ Fingerprint sensor ready on %s\n", SERIAL_PORT);
    
    sleep(2);
    
    // Main loop
    int running = 1;
    while (running) {
        print_main_menu();
        
        char choice[10];
        fgets(choice, sizeof(choice), stdin);
        
        switch (choice[0]) {
            case '1':
                register_client();
                break;
                
            case '2':
                register_product();
                break;
                
            case '3':
                list_clients();
                break;
                
            case '4':
                list_products();
                break;
                
            case '5':
                print_header("FINGERPRINT SENSOR TEST");
                fingerprint_test();
                wait_enter();
                break;
                
            case '6':
                print_header("SYSTEM STATISTICS");
                // Add statistics display
                wait_enter();
                break;
                
            case '7':
                running = 0;
                break;
                
            default:
                printf("\n✗ Invalid choice!\n");
                sleep(1);
                break;
        }
    }
    
    // Cleanup
    fingerprint_close();
    db_close();
    
    printf("\nSystem stopped.\n");
    return 0;
}