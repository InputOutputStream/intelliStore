#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include "mongoose.h"
#include <iostream>
#include <vector>
#include <filesystem>  // C++17
#include <regex>

using namespace cv;
using namespace cv::face;
using namespace std;
namespace fs = std::filesystem;

Ptr<LBPHFaceRecognizer> model = LBPHFaceRecognizer::create();

// Charger les images d'entraînement depuis /images/clients
void train_model(const string& folder_path = "../images/clients") {
    vector<Mat> images;
    vector<int> labels;

    cout << "[INFO] Recherche des images dans : " << folder_path << endl;

    // Parcours des fichiers du dossier
    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (!entry.is_regular_file()) continue;

        string file_path = entry.path().string();
        string filename = entry.path().filename().string();

        // Utilisation d'un regex pour récupérer l'ID du client depuis le nom du fichier : ex "1.jpg" -> 1
        regex re(R"((\d+)\.jpg)");
        smatch match;
        if (regex_match(filename, match, re)) {
            int label = stoi(match[1].str());
            Mat img = imread(file_path, IMREAD_GRAYSCALE);
            if (!img.empty()) {
                images.push_back(img);
                labels.push_back(label);
                cout << "[INFO] Image chargée : " << filename << " | ID: " << label << endl;
            } else {
                cout << "[WARN] Impossible de lire l'image : " << filename << endl;
            }
        }
    }

    if (!images.empty()) {
        model->train(images, labels);
        cout << "[INFO] Modèle entraîné avec succès sur " << images.size() << " images." << endl;
    } else {
        cerr << "[ERROR] Aucune image trouvée pour l'entraînement." << endl;
    }
}

static void handle_request(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_match(hm->uri, mg_str("/identify"), NULL)) {
            // Extraire le chemin de l'image du corps JSON
            char path[256];
            mg_http_get_var(&hm->body, "path", path, sizeof(path));

            Mat test_img = imread(path, IMREAD_GRAYSCALE);
            if (test_img.empty()) {
                mg_http_reply(c, 400, "", "{\"error\": \"Image non trouvée\"}");
                return;
            }

            int label = -1;
            double confidence = 0.0;
            model->predict(test_img, label, confidence);

            // LOG DE DÉBOGAGE
            cout << "[DEBUG] Prediction - ID: " << label << " | Confiance: " << confidence << endl;

            // Seuil LBPH : plus c'est bas, plus la ressemblance est forte
            if (confidence < 120.0) {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"client_id\": %d}", label);
            } else {
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"client_id\": null}");
            }
        }
    }
}

int main() {
    train_model();  // Entraînement automatique depuis le dossier

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", handle_request, NULL);

    cout << "API C++ de reconnaissance lancée sur le port 8000..." << endl;

    for (;;) mg_mgr_poll(&mgr, 1000);
    return 0;
}
