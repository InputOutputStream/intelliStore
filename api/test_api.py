"""
Client de test pour l'API REST
Démontre l'utilisation de tous les endpoints
"""

import requests
import json
from datetime import datetime

API_URL = "http://localhost:5000"

def print_section(title):
    """Afficher un titre de section"""
    print("\n" + "=" * 60)
    print(f" {title}")
    print("=" * 60 + "\n")

def test_health():
    """Tester l'état de l'API"""
    print_section("TEST: État de l'API")
    
    response = requests.get(f"{API_URL}/api/health")
    print(f"Status: {response.status_code}")
    print(json.dumps(response.json(), indent=2))

def test_get_users():
    """Récupérer tous les utilisateurs"""
    print_section("TEST: Récupérer tous les utilisateurs")
    
    response = requests.get(f"{API_URL}/api/users")
    print(f"Status: {response.status_code}")
    data = response.json()
    print(f"Nombre d'utilisateurs: {data['count']}")
    
    if data['users']:
        print("\nUtilisateurs:")
        for user in data['users']:
            print(f"  - {user['name']} (ID: {user['user_id']}, Empreinte: {user['fingerprint_id']})")
    else:
        print("Aucun utilisateur dans la base")

def test_get_logs():
    """Récupérer les logs"""
    print_section("TEST: Récupérer les logs")
    
    response = requests.get(f"{API_URL}/api/logs?limit=10")
    print(f"Status: {response.status_code}")
    data = response.json()
    print(f"Nombre de logs: {data['count']}")
    
    if data['logs']:
        print("\nDernières entrées:")
        for log in data['logs']:
            print(f"  - {log.get('name', 'Inconnu')} à {log['timestamp']}")
    else:
        print("Aucun log enregistré")

def test_get_stats():
    """Récupérer les statistiques"""
    print_section("TEST: Statistiques du système")
    
    response = requests.get(f"{API_URL}/api/stats")
    print(f"Status: {response.status_code}")
    data = response.json()
    
    print(f"Total utilisateurs: {data['total_users']}")
    print(f"Total entrées: {data['total_logs']}")
    print(f"Entrées aujourd'hui: {data['today_logs']}")
    
    if data['top_users']:
        print("\nTop 5 utilisateurs:")
        for user in data['top_users']:
            print(f"  - {user['name']}: {user['entries']} entrées")

def test_create_user():
    """Créer un utilisateur via l'API"""
    print_section("TEST: Créer un utilisateur")
    
    new_user = {
        "name": "Test API",
        "fingerprint_id": 999
    }
    
    print(f"Création de l'utilisateur: {new_user['name']}")
    
    response = requests.post(
        f"{API_URL}/api/users",
        json=new_user,
        headers={"Content-Type": "application/json"}
    )
    
    print(f"Status: {response.status_code}")
    
    if response.status_code == 201:
        print("✓ Utilisateur créé avec succès!")
        print(json.dumps(response.json(), indent=2))
    elif response.status_code == 409:
        print("⚠ L'ID d'empreinte existe déjà")
    else:
        print("✗ Erreur lors de la création")
        print(response.json())

def test_create_log():
    """Créer un log d'entrée"""
    print_section("TEST: Créer un log d'entrée")
    
    # D'abord récupérer un utilisateur existant
    response = requests.get(f"{API_URL}/api/users")
    users = response.json()['users']
    
    if not users:
        print("Aucun utilisateur disponible pour créer un log")
        return
    
    user_id = users[0]['user_id']
    print(f"Création d'un log pour l'utilisateur ID: {user_id}")
    
    response = requests.post(
        f"{API_URL}/api/logs",
        json={"user_id": user_id},
        headers={"Content-Type": "application/json"}
    )
    
    print(f"Status: {response.status_code}")
    
    if response.status_code == 201:
        print("✓ Log créé avec succès!")
        print(json.dumps(response.json(), indent=2))
    else:
        print("✗ Erreur lors de la création du log")
        print(response.json())

def main():
    print("\n" + "=" * 60)
    print(" CLIENT DE TEST - API REST")
    print(" URL de base: " + API_URL)
    print("=" * 60)
    
    try:
        # Tester l'état de l'API
        test_health()
        
        # Tester la récupération des utilisateurs
        test_get_users()
        
        # Tester la récupération des logs
        test_get_logs()
        
        # Tester les statistiques
        test_get_stats()
        
        # Test de création (commentez si vous ne voulez pas créer de données)
        # test_create_user()
        # test_create_log()
        
        print_section("TESTS TERMINÉS")
        print("✓ Tous les tests ont été exécutés avec succès!")
        
    except requests.exceptions.ConnectionError:
        print("\n✗ ERREUR: Impossible de se connecter à l'API")
        print("Vérifiez que l'API est démarrée (python rest_api.py)")
    except Exception as e:
        print(f"\n✗ ERREUR: {e}")

if __name__ == "__main__":
    main()
