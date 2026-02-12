import cv2
import requests
import mysql.connector
import serial
import os
import time
from fpdf import FPDF

# --- CONFIGURATION ---
DB_CONFIG = {
    'user': 'smart_admin',
    'password': 'MonMotDePasse123!',
    'host': 'localhost',
    'database': 'smart_store'
}
PATHS = {
    'invoices': '../invoices',
    'temp_faces': '../images/temp',
    'temp_produit': '../images/temp_produit',
    'cascade': cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
}
CPP_SERVER_URL = "http://localhost:8000/identify"

# Nouvelle URL pour la reconnaissance de produit
CPP_SERVER_URL_Produit = "http://localhost:8080/identify_produit"


class DatabaseManager:
    """Gère toutes les interactions avec la base de données MySQL."""
    def __init__(self, config):
        try:
            self.db = mysql.connector.connect(**config)
            self.cursor = self.db.cursor(dictionary=True)
        except mysql.connector.Error as e:
            print(f"Erreur de connexion DB : {e}")
            exit(1)

    def execute_query(self, query, params=(), commit=False):
        try:
            self.cursor.execute(query, params)
            if commit:
                self.db.commit()
            return self.cursor.fetchall() if not commit else None
        except mysql.connector.Error as e:
            print(f" Erreur SQL : {e}")
            return None

    def close(self):
        self.cursor.close()
        self.db.close()

class InvoiceManager:
    """Gère la création de documents PDF."""
    @staticmethod
    def generate_pdf(client_id, client_name, items, total):
        pdf = FPDF()
        pdf.add_page()
        pdf.set_font("Arial", 'B', 20)
        pdf.cell(190, 20, "FACTURE SMART STORE", ln=True, align='C')
        
        pdf.set_font("Arial", size=12)
        pdf.cell(190, 10, f"Client: {client_name} (ID: {client_id})", ln=True)
        pdf.cell(190, 10, "-"*50, ln=True)

        for item in items:
            pdf.cell(140, 10, item['libelle'])
            pdf.cell(50, 10, f"{item['prix']} EUR", ln=True)

        pdf.cell(190, 10, "-"*50, ln=True)
        pdf.set_font("Arial", 'B', 14)
        pdf.cell(140, 10, "TOTAL")
        pdf.cell(50, 10, f"{total} EUR", ln=True)

        if not os.path.exists(PATHS['invoices']):
            os.makedirs(PATHS['invoices'])
            
        filename = f"{PATHS['invoices']}/facture_{client_id}_{int(time.time())}.pdf"
        pdf.output(filename)
        return filename

class SmartStore:
    def __init__(self):
        self.db_manager = DatabaseManager(DB_CONFIG)
        self.face_cascade = cv2.CascadeClassifier(PATHS['cascade'])
        self.produit_cascade = cv2.CascadeClassifier(PATHS['cascade'])  # temporaire
        self.last_added = {}  # Pour éviter les ajouts multiples (debounce)
        
        self.detection_count = 0 # Compteur pour éviter les détections multiples
        self.last_detected_id = None # Dernier ID détecté pour le produit

    def _get_client_id_from_vision(self, image_path):
        """Communique avec le module C++ pour identifier le client."""
        try:
            r = requests.post(CPP_SERVER_URL, data={'path': image_path}, timeout=1.5)
            return r.json().get('client_id')
        except Exception:
            return None
        
        
    def _get_produit_id_from_vision(self, image_path):
        """Communique avec le module C++ pour identifier le produit."""
        try:
            r = requests.post(CPP_SERVER_URL_Produit, data={'path': image_path}, timeout=1.5)
            return r.json().get('produit_id')
        except Exception:
            return None

    def add_to_cart(self, client_id, product_id):
        now = time.time()
        key = (client_id, product_id)
        
        # Anti-rebond : 10 secondes entre deux ajouts du même produit
        if key not in self.last_added or (now - self.last_added[key]) > 120:
            query = "INSERT INTO paniers_actifs (id_client, id_produit) VALUES (%s, %s)"
            self.db_manager.execute_query(query, (client_id, product_id), commit=True)
            self.last_added[key] = now
            print(f"🛒 Produit {product_id} ajouté au client {client_id}")

    def get_cart_details(self, client_id):
        query = """
            SELECT p.libelle, p.prix FROM produits p
            JOIN paniers_actifs pa ON p.id_produit = pa.id_produit
            WHERE pa.id_client = %s
        """
        items = self.db_manager.execute_query(query, (client_id,))
        total = sum(item['prix'] for item in items) if items else 0
        return items, total

    def finalize_transaction(self, client_id):
        items, total = self.get_cart_details(client_id)
        if not items:
            print("Panier vide.")
            return

        # Récupération nom client
        res = self.db_manager.execute_query("SELECT nom FROM clients WHERE id_client = %s", (client_id,))
        client_name = res[0]['nom'] if res else "Inconnu"

        # Facturation
        file_path = InvoiceManager.generate_pdf(client_id, client_name, items, total)
        
        # Archive et Nettoyage
        self.db_manager.execute_query(
            "INSERT INTO factures (id_client, montant_total, statut_paiement) VALUES (%s, %s, 'paye')",
            (client_id, total), commit=True
        )
        self.db_manager.execute_query("DELETE FROM paniers_actifs WHERE id_client = %s", (client_id,), commit=True)
        
        print(f"Transaction validée. Facture : {file_path}")

    def handle_payment(self, client_id):
        print(f"\n--- ZONE DE PAIEMENT : CLIENT {client_id} ---")
        _, total = self.get_cart_details(client_id)
        print(f"Montant à régler : {total} EUR")

        # Tentative Serial
        ser = None
        try:
            ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=0.1)
        except Exception:
            print("Capteur absent. Mode simulation : [v] Valider, [c] Annuler")

        while True:
            # Logique de validation (Réelle ou Simulation)
            if ser and ser.in_waiting > 0:
                finger_id = ser.readline().decode('utf-8').strip()
                # Logique simplifiée ici pour l'exemple
                self.finalize_transaction(client_id)
                break

            key = cv2.waitKey(1) & 0xFF
            if key == ord('v'): 
                self.finalize_transaction(client_id)
                break
            elif key == ord('c'):
                print("Annulé.")
                break

    def run(self):
        cap = cv2.VideoCapture(0)
        
        while True:
            ret, frame = cap.read()
            if not ret: break
            
            h_frame, w_frame, _ = frame.shape
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

            # --- PARTIE 1 : RECONNAISSANCE DU CLIENT (VISAGE) ---
            faces = self.face_cascade.detectMultiScale(gray, 1.3, 5)
            current_client = None
            for (x, y, w, h) in faces:
                # Identification du client (ton code actuel)
                face_img = gray[y:y+h, x:x+w]
                img_path = os.path.join(PATHS['temp_faces'], "current_face.jpg")
                cv2.imwrite(img_path, cv2.resize(face_img, (200, 200)))
                current_client = self._get_client_id_from_vision(img_path)
                
                color = (0, 255, 0) if current_client else (0, 0, 255)
                cv2.rectangle(frame, (x, y), (x+w, y+h), color, 2)
                cv2.putText(frame, f"Client: {current_client}", (x, y-10), 1, 1.5, color, 2)

            # --- PARTIE 2 : ZONE DE SCAN PRODUIT (RECTANGLE FIXE) ---
            # On définit un carré au milieu de l'écran
            box_size = 250
            margin = 20
            x1, y1 = margin, margin
            x2, y2 = x1 + box_size, y1 + box_size

            # x1, y1 = (w_frame//2 - box_size//2), (h_frame//2 - box_size//2)
            # x2, y2 = x1 + box_size, y1 + box_size
            
            # Dessiner la zone de scan en bleu
            cv2.rectangle(frame, (x1, y1), (x2, y2), (255, 0, 0), 2)
            cv2.putText(frame, "SCAN PRODUIT", (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)
            # cv2.putText(frame, "PLACEZ LE PRODUIT ICI", (x1, y1-10), 1, 1.2, (255, 0, 0), 2)
                
            # Si un client est présent, on analyse ce qu'il y a dans le carré bleu
            if current_client:
                roi_produit = frame[y1:y2, x1:x2] # On découpe la zone du produit
                prod_path = os.path.join(PATHS['temp_produit'], "scan_produit.jpg")
                cv2.imwrite(prod_path, roi_produit)
                
                # Appel à ton serveur C++ pour le produit
                produit_id = self._get_produit_id_from_vision(prod_path)
                if produit_id:
                    if produit_id == self.last_detected_id:
                        self.detection_count += 1
                    else:
                        self.detection_count = 1
                        self.last_detected_id = produit_id
            
                    if self.detection_count == 20: # Si le même produit est détecté pendant 15 frames consécutives (~0.5s), on l'ajoute au panier
                        self.add_to_cart(current_client, produit_id)
                        cv2.putText(frame, f"PRODUIT DETECTE: {produit_id}", (x1, y2+30), 1, 1.5, (0, 255, 255), 2)
                        self.detection_count = -100 # Reset pour éviter les ajouts multiples
                else:
                    cv2.putText(frame, "Aucun produit détecté", (x1, y2+30), 1, 1.5, (0, 0, 255), 2)
                    self.detection_count = 0
                    self.last_detected_id = None
                    


            cv2.imshow('Smart Store', frame)
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'): break
            if key == ord('p') and current_client:
                self.handle_payment(current_client)

        cap.release()
        cv2.destroyAllWindows()
        self.db_manager.close()



if __name__ == "__main__":
    store = SmartStore()
    store.run()