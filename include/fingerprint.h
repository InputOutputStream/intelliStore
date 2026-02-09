#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <stdint.h>
#include <stdlib.h>

// Codes de retour du capteur
#define FINGERPRINT_OK          0x00
#define FINGERPRINT_NOFINGER    0x02
#define FINGERPRINT_NOTFOUND    0x09

// Structure pour un utilisateur
typedef struct {
    int user_id;
    char name[100];
    int fingerprint_id;
} User;

// Initialisation
int fingerprint_init(const char* port);
void fingerprint_close();

// Fonctions principales
int fingerprint_enroll(const char* name);
int fingerprint_recognize();
int fingerprint_test();
/* Fonction pour identifier et envoyer infos */
int fingerprint_identify_and_notify();

// Utilitaires
int get_next_available_id();

#endif