#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <curl/curl.h>
#include "../include/fingerprint.h"
#include "../include/database.h"

// Dépendance externe
extern int serial_open(const char* port);
extern void serial_close();
extern int serial_write(uint8_t* data, int len);
extern int serial_read(uint8_t* buffer, int len, int timeout_ms);

// Buffer pour packets
static uint8_t packet[256];

// Constantes du capteur
#define FINGERPRINT_GETIMAGE    0x01
#define FINGERPRINT_IMAGE2TZ    0x02
#define FINGERPRINT_SEARCH      0x04
#define FINGERPRINT_REGMODEL    0x05
#define FINGERPRINT_STORE       0x06
#define FINGERPRINT_DELETE      0x0C
#define FINGERPRINT_VERIFYPASSWORD 0x13
#define FINGERPRINT_TEMPLATECOUNT 0x1D

// ==================== FONCTIONS PRIVEES ====================

static uint16_t calculate_checksum(uint8_t* data, int len) {
    uint16_t sum = 0;
    for (int i = 0; i < len; i++) sum += data[i];
    return sum;
}

static void build_packet(uint8_t cmd, uint8_t* params, int plen) {
    int idx = 0;
    packet[idx++] = 0xEF; packet[idx++] = 0x01;
    packet[idx++] = 0xFF; packet[idx++] = 0xFF;
    packet[idx++] = 0xFF; packet[idx++] = 0xFF;
    packet[idx++] = 0x01;
    
    uint16_t length = 1 + plen + 2;
    packet[idx++] = (length >> 8) & 0xFF;
    packet[idx++] = length & 0xFF;
    packet[idx++] = cmd;
    
    for (int i = 0; i < plen; i++) packet[idx++] = params[i];
    
    uint16_t checksum = calculate_checksum(&packet[6], idx - 6);
    packet[idx++] = (checksum >> 8) & 0xFF;
    packet[idx++] = checksum & 0xFF;
}

static int send_cmd(uint8_t cmd, uint8_t* params, int plen, uint8_t* resp) {
    build_packet(cmd, params, plen);
    int sent = serial_write(packet, 12 + plen);
    if (sent <= 0) return -1;
    
    usleep(200000);  // 200ms = 200000 microseconds
    return serial_read(resp, 32, 1000);
}

static long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

static int wait_finger(int timeout_ms) {
    uint8_t resp[32];
    long start = get_time_ms();
    
    printf("   ");
    fflush(stdout);
    
    while ((get_time_ms() - start) < timeout_ms) {
        build_packet(FINGERPRINT_GETIMAGE, NULL, 0);
        serial_write(packet, 12);
        
        usleep(100000);  // 100ms
        int read = serial_read(resp, 32, 300);
        
        if (read >= 12) {
            if (resp[9] == FINGERPRINT_OK) {
                printf("✓\n");
                return 0;
            }
        }
        
        // Afficher un point toutes les secondes
        if (((get_time_ms() - start) % 1000) < 100) {
            printf(".");
            fflush(stdout);
        }
        usleep(200000);  // 200ms
    }
    
    printf(" Timeout\n");
    return -1;
}

// ==================== FONCTIONS PUBLIQUES ====================

int fingerprint_init(const char* port) {
    if (serial_open(port) != 0) {
        return -1;
    }
    
    // Attendre que l'Arduino soit pret
    usleep(2000000);  // 2 seconds = 2000000 microseconds
    
    // Tester la communication
    uint8_t resp[32];
    uint8_t pass[] = {0x00, 0x00, 0x00, 0x00};
    
    if (send_cmd(FINGERPRINT_VERIFYPASSWORD, pass, 4, resp) < 12 ||
        resp[9] != FINGERPRINT_OK) {
        serial_close();
        return -1;
    }
    
    return 0;
}

void fingerprint_close() {
    serial_close();
}

int fingerprint_test() {
    uint8_t resp[32];
    
    // Test password
    uint8_t pass[] = {0x00, 0x00, 0x00, 0x00};
    if (send_cmd(FINGERPRINT_VERIFYPASSWORD, pass, 4, resp) < 12 ||
        resp[9] != FINGERPRINT_OK) {
        return -1;
    }
    
    // Get template count
    if (send_cmd(FINGERPRINT_TEMPLATECOUNT, NULL, 0, resp) >= 12 &&
        resp[9] == FINGERPRINT_OK) {
        int count = (resp[10] << 8) | resp[11];
        printf("   Empreintes stockees: %d\n", count);
        return 0;
    }
    
    return -1;
}

int fingerprint_enroll(const char* name) {
    uint8_t resp[32];
    
    // Trouver le prochain ID disponible
    int id = get_next_available_id();
    if (id <= 0 || id > 163) {
        printf("❌ Plus d'ID disponible\n");
        return -1;
    }
    
    printf("   Enregistrement ID: %d\n", id);
    printf("   1. Posez votre doigt");
    fflush(stdout);
    
    // Premiere capture
    if (wait_finger(10000) != 0) return -1;
    usleep(500000);  // 500ms
    
    uint8_t param1[] = {0x01};
    if (send_cmd(FINGERPRINT_IMAGE2TZ, param1, 1, resp) < 12 || 
        resp[9] != FINGERPRINT_OK) {
        printf("   ❌ Erreur capture 1\n");
        return -1;
    }
    
    printf("   2. Retirez votre doigt\n");
    usleep(2000000);  // 2 seconds
    
    printf("   3. Reposez le meme doigt");
    fflush(stdout);
    
    // Deuxieme capture
    if (wait_finger(10000) != 0) return -1;
    usleep(500000);  // 500ms
    
    uint8_t param2[] = {0x02};
    if (send_cmd(FINGERPRINT_IMAGE2TZ, param2, 1, resp) < 12 || 
        resp[9] != FINGERPRINT_OK) {
        printf("   ❌ Erreur capture 2\n");
        return -1;
    }
    
    // Creer modele
    if (send_cmd(FINGERPRINT_REGMODEL, NULL, 0, resp) < 12 || 
        resp[9] != FINGERPRINT_OK) {
        printf("   ❌ Les empreintes ne correspondent pas\n");
        return -1;
    }
    
    // Stocker avec ID
    uint8_t store_params[] = {0x01, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF), 0x00};
    if (send_cmd(FINGERPRINT_STORE, store_params, 4, resp) < 12 || 
        resp[9] != FINGERPRINT_OK) {
        printf("   ❌ Erreur stockage\n");
        return -1;
    }
    
    // Enregistrer dans la base de donnees
    if (db_create_user(name, id) <= 0) {
        printf("   ❌ Erreur enregistrement base de donnees\n");
        return -1;
    }
    
    return id;
}

int fingerprint_recognize() {
    uint8_t resp[32];
    
    printf("   ");
    fflush(stdout);
    if (wait_finger(10000) != 0) return -1;
    
    usleep(500000);  // 500ms
    
    // Convertir image
    uint8_t param[] = {0x01};
    if (send_cmd(FINGERPRINT_IMAGE2TZ, param, 1, resp) < 12 || 
        resp[9] != FINGERPRINT_OK) {
        return -1;
    }
    
    // Rechercher
    uint8_t search_params[] = {0x01, 0x00, 0x00, 0x00, 0xA3, 0x00};
    if (send_cmd(FINGERPRINT_SEARCH, search_params, 6, resp) < 16) {
        return -1;
    }
    
    if (resp[9] == FINGERPRINT_OK) {
        return (resp[10] << 8) | resp[11];
    }
    
    return -1;
}

int get_next_available_id() {
    // Chercher un ID libre entre 1 et 163
    for (int id = 1; id <= 163; id++) {
        User user;
        if (db_get_user_by_fingerprint(id, &user) != 0) {
            // Aucun utilisateur avec cet ID fingerprint
            return id;
        }
    }
    return -1;
}

// Structure pour stocker la réponse HTTP
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback pour écrire la réponse
static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("❌ Erreur d'allocation mémoire\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

int fingerprint_identify_and_notify() {
    // D'abord identifier l'empreinte
    printf("Posez votre doigt sur le capteur...\n\n");
    
    int fingerprint_id = fingerprint_recognize();
    
    if (fingerprint_id <= 0) {
        printf("\n❌ Empreinte non reconnue\n");
        return -1;
    }
    
    // Récupérer les infos de l'utilisateur
    User user;
    if (db_get_user_by_fingerprint(fingerprint_id, &user) != 0) {
        printf("\n⚠ Empreinte reconnue mais utilisateur introuvable\n");
        printf("   ID Empreinte: %d\n", fingerprint_id);
        return -1;
    }
    
    printf("\n✅ Utilisateur identifié: %s\n", user.name);
    printf("   ID: %d\n", user.user_id);
    printf("   ID Empreinte: %d\n", fingerprint_id);
    
    // Enregistrer l'entrée dans la base de données locale
    db_log_entry(user.user_id);
    printf("\n✅ Entrée enregistrée localement\n");
    
    // Préparer le JSON
    char json_body[256];
    snprintf(json_body, sizeof(json_body), "{\"fingerprint_id\":%d}", fingerprint_id);
    
    // Initialiser libcurl
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if (!curl) {
        printf("\n⚠ Impossible d'initialiser HTTP client\n");
        printf("   Données enregistrées localement seulement\n");
        free(chunk.memory);
        curl_global_cleanup();
        return 1;  // Succès local, échec réseau
    }
    
    // Configurer la requête
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/api/identify");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);  // 5 seconds timeout
    
    // Envoyer la requête
    printf("\nEnvoi des informations à l'API...\n");
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        printf("\n⚠ Impossible de contacter l'API: %s\n", curl_easy_strerror(res));
        printf("   Vérifiez que l'API est démarrée (./start_api.sh)\n");
        printf("   Données enregistrées localement seulement\n");
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(chunk.memory);
        curl_global_cleanup();
        return 1;  // Succès local, échec réseau
    }
    
    // Vérifier le code de réponse
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code == 200) {
        printf("\n✅ Informations envoyées avec succès à l'API\n");
        if (chunk.size > 0) {
            printf("   Réponse: %s\n", chunk.memory);
        }
    } else {
        printf("\n⚠ Erreur API (code %ld)\n", response_code);
        printf("   Données enregistrées localement seulement\n");
    }
    
    // Nettoyage
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(chunk.memory);
    curl_global_cleanup();
    
    return (response_code == 200) ? 0 : 1;
}