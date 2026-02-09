#!/bin/bash

# REST API Startup Script for Linux
# Fingerprint Recognition System

echo "========================================="
echo " API REST - Système d'Empreintes"
echo "========================================="
echo ""

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "ERREUR: Python 3 n'est pas installé"
    echo "Installez-le avec: sudo apt install python3 python3-pip"
    exit 1
fi

echo "Python version:"
python3 --version
echo ""

# Check if pip is installed
if ! command -v pip3 &> /dev/null; then
    echo "ERREUR: pip3 n'est pas installé"
    echo "Installez-le avec: sudo apt install python3-pip"
    exit 1
fi

echo "Installation des dépendances..."
pip3 install -r requirements.txt

if [ $? -ne 0 ]; then
    echo ""
    echo "AVERTISSEMENT: Certaines dépendances n'ont peut-être pas été installées"
    echo "Si vous rencontrez des erreurs, essayez:"
    echo "  pip3 install --user -r requirements.txt"
    echo "ou"
    echo "  sudo pip3 install -r requirements.txt"
    echo ""
fi

echo ""
echo "Démarrage de l'API..."
echo ""

# Run the API
python3 rest_api.py