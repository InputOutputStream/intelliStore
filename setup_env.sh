#!/bin/bash

# --- COULEURS POUR LA LISIBILITÉ ---
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Début de l'installation de l'environnement Smart Store ===${NC}"

# 1. Mise à jour du système
echo -e "${GREEN}[1/7] Mise à jour du système...${NC}"
sudo apt update && sudo apt upgrade -y

# 2. Installation des dépendances système et compilateurs
echo -e "${GREEN}[2/7] Installation des outils de compilation et librairies...${NC}"
sudo apt install -y build-essential cmake git pkg-config \
    libopencv-dev libopencv-contrib-dev \
    python3 python3-pip python3-dev \
    libmysqlclient-dev libssl-dev \
    mysql-server

# 3. Installation des librairies Python
echo -e "${GREEN}[3/7] Installation des librairies Python...${NC}"
pip3 install --upgrade pip
pip3 install opencv-python requests mysql-connector-python fpdf2 pyserial

# 4. Configuration de l'environnement graphique (XCB/Qt)
echo -e "${GREEN}[4/7] Configuration de QT_QPA_PLATFORM...${NC}"
if ! grep -q "QT_QPA_PLATFORM=xcb" ~/.bashrc; then
    echo 'export QT_QPA_PLATFORM=xcb' >> ~/.bashrc
    echo "Variable QT_QPA_PLATFORM ajoutée au .bashrc"
fi
export QT_QPA_PLATFORM=xcb

# 5. Création de l'arborescence des dossiers
echo -e "${GREEN}[5/7] Création des dossiers du projet...${NC}"
mkdir -p ../images/clients
mkdir -p ../images/temp
mkdir -p ../invoices
chmod -R 777 ../images ../invoices

# 6. Configuration de MySQL (Optionnel - Création base de données)
echo -e "${GREEN}[6/7] Configuration de la base de données...${NC}"
sudo service mysql start
# Note : On crée la base et l'utilisateur (à adapter selon tes besoins)
sudo mysql -u root -e "CREATE DATABASE IF NOT EXISTS smart_store;"
sudo mysql -u root -e "CREATE USER IF NOT EXISTS 'smart_admin'@'localhost' IDENTIFIED BY 'MonMotDePasse123!';"
sudo mysql -u root -e "GRANT ALL PRIVILEGES ON smart_store.* TO 'smart_admin'@'localhost';"
sudo mysql -u root -e "FLUSH PRIVILEGES;"

# 7. Vérification finale
echo -e "${GREEN}[7/7] Vérification des versions...${NC}"
echo -n "Python : " && python3 --version
echo -n "OpenCV C++ : " && pkg-config --modversion opencv4
echo -n "MySQL : " && mysql --version

echo -e "${BLUE}=== Installation terminée ! ===${NC}"
echo -e "Veuillez redémarrer votre terminal ou taper : ${GREEN}source ~/.bashrc${NC}"