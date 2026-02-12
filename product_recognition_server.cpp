#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include "external/mongoose.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;  // ✓ Correct
using namespace cv;
using namespace cv::face;
using namespace std;

Ptr<LBPHFaceRecognizer> model = LBPHFaceRecognizer::create();
// Taille standard pour l'entraînement (doit être la même que tes images dans /produits)
const Size TRAINING_SIZE(200, 200); 

void train_model(const string& directory_path) {
    vector<Mat> images;
    vector<int> labels;
    cout << "[INFO] Entraînement du modèle en cours..." << endl;

    try {
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            string path = entry.path().string();
            string filename = entry.path().stem().string();

            try {
                int label = stoi(filename);
                Mat img = imread(path, IMREAD_GRAYSCALE);
                if (!img.empty()) {
                    // On redimensionne pour être sûr de la cohérence
                    resize(img, img, TRAINING_SIZE);
                    images.push_back(img);
                    labels.push_back(label);
                    cout << "  > Chargé : produit " << label << " (" << path << ")" << endl;
                }
            } catch (...) { /* Ignorer fichiers non-ID */ }
        }

        if (images.empty()) {
            cerr << "[ERREUR] Aucune image trouvée !" << endl;
            return;
        }

        model->train(images, labels);
        cout << "[OK] Modèle entraîné avec " << images.size() << " images." << endl;
    } catch (const exception& e) {
        cerr << "[ERREUR FATALE] : " << e.what() << endl;
    }
}

static void handle_request(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        if (mg_match(hm->uri, mg_str("/identify_produit"), NULL)) {
            char path[512];
            mg_http_get_var(&hm->body, "path", path, sizeof(path));

            cout << "[RECU] Analyse de l'image : " << path << endl;

            Mat test_img = imread(path, IMREAD_GRAYSCALE);
            if (test_img.empty()) {
                mg_http_reply(c, 400, "", "{\"error\": \"Image introuvable\"}");
                return;
            }

            // IMPORTANT : Redimensionner l'image reçue à la taille d'entraînement
            resize(test_img, test_img, TRAINING_SIZE);

            int label = -1;
            double confidence = 0.0;
            model->predict(test_img, label, confidence);

            // LOG de debug pour t'aider à régler le seuil
            cout << "[RESULTAT] ID: " << label << " | Confiance (Distance): " << confidence << endl;

            // Ajustement du seuil : Pour LBPH, entre 80 et 150 est souvent nécessaire pour les objets
            if (label != -1 && confidence < 90.0) { 
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"produit_id\": %d, \"confidence\": %.2f}", label, confidence);
            } else {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"produit_id\": null, \"confidence\": %.2f}", confidence);
            }
        }
    }
}

int main() {
    train_model("../images/produits");

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    
    if (mg_http_listen(&mgr, "http://0.0.0.0:8080", handle_request, NULL) == NULL) {
        cerr << "Erreur port 8080" << endl;
        return 1;
    }

    cout << "--- Serveur Reconnaissance Produit actif sur le port 8080 ---" << endl;
    for (;;) mg_mgr_poll(&mgr, 1000);

    return 0;
}