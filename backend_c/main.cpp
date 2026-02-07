#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include "mongoose.h"
#include <iostream>
#include <vector>
#include <filesystem> // Pour scanner le dossier
#include <string>

namespace fs = std::filesystem;
using namespace cv;
using namespace cv::face;
using namespace std;

// Singleton pour le modèle de reconnaissance
Ptr<LBPHFaceRecognizer> model = LBPHFaceRecognizer::create();

/**
 * Charge automatiquement toutes les images du dossier clients.
 * Format attendu : "ID.jpg" ou "ID.png" (ex: 1.jpg, 2.jpg)
 */
void train_model(const string& directory_path) {
    vector<Mat> images;
    vector<int> labels;

    cout << "[INFO] Entraînement du modèle en cours..." << endl;

    try {
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            string path = entry.path().string();
            string filename = entry.path().stem().string(); // Récupère le nom sans extension

            // Convertir le nom du fichier en ID entier
            try {
                int label = stoi(filename);
                Mat img = imread(path, IMREAD_GRAYSCALE);

                if (!img.empty()) {
                    images.push_back(img);
                    labels.push_back(label);
                    cout << "  > Chargé : Client " << label << " (" << path << ")" << endl;
                }
            } catch (const exception& e) {
                cerr << "  [!] Ignoré : " << path << " (le nom doit être un nombre ID)" << endl;
            }
        }

        if (images.empty()) {
            cerr << "[ERREUR] Aucune image trouvée dans " << directory_path << endl;
            return;
        }

        model->train(images, labels);
        cout << "[OK] Modèle entraîné avec " << images.size() << " images." << endl;
    } catch (const exception& e) {
        cerr << "[ERREUR FATALE] Impossible d'accéder au dossier : " << e.what() << endl;
    }
}

/**
 * Gestionnaire des requêtes HTTP (Mongoose)
 */
static void handle_request(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        if (mg_match(hm->uri, mg_str("/identify"), NULL)) {
            char path[512];
            mg_http_get_var(&hm->body, "path", path, sizeof(path));

            Mat test_img = imread(path, IMREAD_GRAYSCALE);
            if (test_img.empty()) {
                mg_http_reply(c, 400, "", "{\"error\": \"Image invalide\"}");
                return;
            }

            int label = -1;
            double confidence = 0.0;
            model->predict(test_img, label, confidence);

            cout << "[LOG] Identification - ID: " << label << " | Confiance: " << confidence << endl;

            // Seuil de confiance LBPH (A ajuster selon l'éclairage)
            if (label != -1 && confidence < 100.0) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"client_id\": %d}", label);
            } else {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"client_id\": null}");
            }
        }
    }
}

int main() {
    // 1. Initialisation et Entraînement
    train_model("../images/clients");

    // 2. Lancement du serveur Web
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    
    if (mg_http_listen(&mgr, "http://0.0.0.0:8000", handle_request, NULL) == NULL) {
        cerr << "Erreur : Impossible de lancer le serveur sur le port 8000" << endl;
        return 1;
    }

    cout << "--- Serveur Reconnaissance prêt sur http://localhost:8000 ---" << endl;

    for (;;) mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    return 0;
}