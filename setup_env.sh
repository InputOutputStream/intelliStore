#!/bin/bash

set -e  # Stop au premier échec

# --- COULEURS POUR LA LISIBILITÉ ---
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}=== Installation de l'environnement Smart Store (version propre) ===${NC}"

# 1. Mise à jour du système
echo -e "${GREEN}[1/7] Mise à jour du système...${NC}"
sudo apt update
sudo apt upgrade -y

# 2. Installation des dépendances système et compilateurs
echo -e "${GREEN}[2/7] Installation des outils et librairies...${NC}"
sudo apt install -y \
    build-essential cmake git pkg-config \
    libopencv-dev libopencv-contrib-dev \
    python3 python3-pip python3-dev \
    libssl-dev \
    mysql-server mysql-client default-libmysqlclient-dev libcurl4-gnutls-dev

# 3. Installation des librairies Python
echo -e "${GREEN}[3/7] Installation des librairies Python...${NC}"
python3 -m pip install --upgrade pip
python3 -m pip install opencv-python requests mysql-connector-python fpdf2 pyserial

# 4. Configuration de l'environnement graphique (XCB/Qt)
echo -e "${GREEN}[4/7] Configuration de QT_QPA_PLATFORM...${NC}"
if ! grep -q "QT_QPA_PLATFORM=xcb" ~/.bashrc; then
    echo 'export QT_QPA_PLATFORM=xcb' >> ~/.bashrc
    echo "Variable QT_QPA_PLATFORM ajoutée au .bashrc"
fi
export QT_QPA_PLATFORM=xcb

# 5. Création de l'arborescence des dossiers
echo -e "${GREEN}[5/7] Création des dossiers du projet...${NC}"
mkdir -p ../images/clients ../images/temp ../invoices
chmod -R 755 ../images ../invoices

# 6. Démarrage et configuration de MySQL
echo -e "${GREEN}[6/7] Configuration de MySQL...${NC}"
sudo systemctl enable mysql
sudo systemctl start mysql

# Attendre que MySQL démarre vraiment
sleep 3

# Création de la base et de l'utilisateur
DB_NAME="smart_store"
DB_USER="smart_admin"
DB_PASS="123456789"

sudo mysql -u root <<EOF
CREATE DATABASE IF NOT EXISTS ${DB_NAME};
CREATE USER IF NOT EXISTS '${DB_USER}'@'localhost' IDENTIFIED BY '${DB_PASS}';
GRANT ALL PRIVILEGES ON ${DB_NAME}.* TO '${DB_USER}'@'localhost';
FLUSH PRIVILEGES;
EOF

# 7. Vérification finale
echo -e "${GREEN}[7/7] Vérification des versions...${NC}"
echo -n "Python : " && python3 --version
echo -n "OpenCV C++ : " && pkg-config --modversion opencv4 || echo "OpenCV non trouvé via pkg-config"
echo -n "MySQL : " && mysql --version

echo -e "${BLUE}=== Installation terminée avec succès ! ===${NC}"
echo -e "Redémarre ton terminal ou exécute : ${GREEN}source ~/.bashrc${NC}"
