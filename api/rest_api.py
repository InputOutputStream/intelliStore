"""
API REST pour le système de reconnaissance d'empreintes digitales
Permet d'accéder aux données via HTTP
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
import sqlite3
from datetime import datetime
import os

app = Flask(__name__)
CORS(app)  # Permettre les requêtes cross-origin

# Configuration
DB_PATH = '../database/fingerprints.db'

def get_db_connection():
    """Créer une connexion à la base de données"""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

@app.route('/')
def index():
    """Page d'accueil de l'API"""
    return jsonify({
        'message': 'API Système de Reconnaissance d\'Empreintes Digitales',
        'version': '1.0',
        'endpoints': {
            'users': '/api/users',
            'logs': '/api/logs',
            'stats': '/api/stats'
        }
    })

@app.route('/api/users', methods=['GET'])
def get_users():
    """Récupérer tous les utilisateurs"""
    conn = get_db_connection()
    users = conn.execute('SELECT * FROM users ORDER BY user_id DESC').fetchall()
    conn.close()
    
    return jsonify({
        'count': len(users),
        'users': [dict(user) for user in users]
    })

@app.route('/api/users/<int:user_id>', methods=['GET'])
def get_user(user_id):
    """Récupérer un utilisateur spécifique"""
    conn = get_db_connection()
    user = conn.execute('SELECT * FROM users WHERE user_id = ?', (user_id,)).fetchone()
    conn.close()
    
    if user is None:
        return jsonify({'error': 'Utilisateur non trouvé'}), 404
    
    return jsonify(dict(user))

@app.route('/api/logs', methods=['GET'])
def get_logs():
    """Récupérer tous les logs"""
    # Paramètres optionnels
    limit = request.args.get('limit', 100, type=int)
    user_id = request.args.get('user_id', type=int)
    
    conn = get_db_connection()
    
    if user_id:
        query = '''
            SELECT l.*, u.name 
            FROM logs l
            LEFT JOIN users u ON l.user_id = u.user_id
            WHERE l.user_id = ?
            ORDER BY l.timestamp DESC
            LIMIT ?
        '''
        logs = conn.execute(query, (user_id, limit)).fetchall()
    else:
        query = '''
            SELECT l.*, u.name 
            FROM logs l
            LEFT JOIN users u ON l.user_id = u.user_id
            ORDER BY l.timestamp DESC
            LIMIT ?
        '''
        logs = conn.execute(query, (limit,)).fetchall()
    
    conn.close()
    
    return jsonify({
        'count': len(logs),
        'logs': [dict(log) for log in logs]
    })

@app.route('/api/logs/<int:log_id>', methods=['GET'])
def get_log(log_id):
    """Récupérer un log spécifique"""
    conn = get_db_connection()
    log = conn.execute('''
        SELECT l.*, u.name 
        FROM logs l
        LEFT JOIN users u ON l.user_id = u.user_id
        WHERE l.log_id = ?
    ''', (log_id,)).fetchone()
    conn.close()
    
    if log is None:
        return jsonify({'error': 'Log non trouvé'}), 404
    
    return jsonify(dict(log))

@app.route('/api/stats', methods=['GET'])
def get_stats():
    """Récupérer les statistiques du système"""
    conn = get_db_connection()
    
    # Nombre total d'utilisateurs
    total_users = conn.execute('SELECT COUNT(*) as count FROM users').fetchone()['count']
    
    # Nombre total d'entrées
    total_logs = conn.execute('SELECT COUNT(*) as count FROM logs').fetchone()['count']
    
    # Entrées aujourd'hui
    today_logs = conn.execute('''
        SELECT COUNT(*) as count FROM logs 
        WHERE DATE(timestamp) = DATE('now')
    ''').fetchone()['count']
    
    # Top 5 des utilisateurs les plus actifs
    top_users = conn.execute('''
        SELECT u.user_id, u.name, COUNT(l.log_id) as entries
        FROM users u
        LEFT JOIN logs l ON u.user_id = l.user_id
        GROUP BY u.user_id
        ORDER BY entries DESC
        LIMIT 5
    ''').fetchall()
    
    # Dernières entrées
    recent_logs = conn.execute('''
        SELECT l.*, u.name
        FROM logs l
        LEFT JOIN users u ON l.user_id = u.user_id
        ORDER BY l.timestamp DESC
        LIMIT 10
    ''').fetchall()
    
    conn.close()
    
    return jsonify({
        'total_users': total_users,
        'total_logs': total_logs,
        'today_logs': today_logs,
        'top_users': [dict(user) for user in top_users],
        'recent_activity': [dict(log) for log in recent_logs]
    })

@app.route('/api/users', methods=['POST'])
def create_user():
    """Créer un nouvel utilisateur (via API)"""
    data = request.get_json()
    
    if not data or 'name' not in data or 'fingerprint_id' not in data:
        return jsonify({'error': 'Données manquantes (name, fingerprint_id requis)'}), 400
    
    conn = get_db_connection()
    
    try:
        cursor = conn.execute(
            'INSERT INTO users (name, fingerprint_id) VALUES (?, ?)',
            (data['name'], data['fingerprint_id'])
        )
        conn.commit()
        user_id = cursor.lastrowid
        
        user = conn.execute('SELECT * FROM users WHERE user_id = ?', (user_id,)).fetchone()
        conn.close()
        
        return jsonify({
            'message': 'Utilisateur créé avec succès',
            'user': dict(user)
        }), 201
        
    except sqlite3.IntegrityError:
        conn.close()
        return jsonify({'error': 'L\'ID d\'empreinte existe déjà'}), 409

@app.route('/api/logs', methods=['POST'])
def create_log():
    """Créer un nouveau log d'entrée"""
    data = request.get_json()
    
    if not data or 'user_id' not in data:
        return jsonify({'error': 'user_id requis'}), 400
    
    conn = get_db_connection()
    
    # Vérifier que l'utilisateur existe
    user = conn.execute('SELECT * FROM users WHERE user_id = ?', (data['user_id'],)).fetchone()
    
    if user is None:
        conn.close()
        return jsonify({'error': 'Utilisateur non trouvé'}), 404
    
    cursor = conn.execute(
        'INSERT INTO logs (user_id) VALUES (?)',
        (data['user_id'],)
    )
    conn.commit()
    log_id = cursor.lastrowid
    
    log = conn.execute('SELECT * FROM logs WHERE log_id = ?', (log_id,)).fetchone()
    conn.close()
    
    return jsonify({
        'message': 'Entrée enregistrée avec succès',
        'log': dict(log)
    }), 201

@app.route('/api/health', methods=['GET'])
def health():
    """Vérifier l'état de l'API et de la base de données"""
    try:
        conn = get_db_connection()
        conn.execute('SELECT 1').fetchone()
        conn.close()
        db_status = 'OK'
    except:
        db_status = 'ERROR'
    
    return jsonify({
        'api_status': 'OK',
        'database_status': db_status,
        'timestamp': datetime.now().isoformat()
    })

# ==================== ROUTE SIMPLE POUR IDENTIFICATION ====================

@app.route('/api/identify', methods=['POST'])
def api_identify():
    """IDENTIFIER UN UTILISATEUR ET ENVOYER SES INFOS AU SYSTÈME EXTERNE"""
    data = request.get_json()
    
    if not data or 'fingerprint_id' not in data:
        return jsonify({'error': 'fingerprint_id requis'}), 400
    
    conn = get_db_connection()
    
    # Trouver l'utilisateur
    user = conn.execute(
        'SELECT * FROM users WHERE fingerprint_id = ?', 
        (data['fingerprint_id'],)
    ).fetchone()
    
    if user is None:
        conn.close()
        return jsonify({'error': 'Utilisateur non trouvé'}), 404
    
    user_dict = dict(user)
    conn.close()
    
    # SIMPLE: Retourner juste les infos
    return jsonify({
        'success': True,
        'message': f'Utilisateur identifié: {user_dict["name"]}',
        'user': user_dict,
        'timestamp': datetime.now().isoformat()
    }), 200

if __name__ == '__main__':
    # Créer le dossier database s'il n'existe pas
    os.makedirs('../database', exist_ok=True)
    
    print("=" * 50)
    print(" API REST - Système de Reconnaissance d'Empreintes")
    print("=" * 50)
    print()
    print(" URL de base: http://localhost:5000")
    print()
    print(" Endpoints disponibles:")
    print("   GET  /api/users          - Liste des utilisateurs")
    print("   GET  /api/users/<id>     - Détails d'un utilisateur")
    print("   POST /api/users          - Créer un utilisateur")
    print("   GET  /api/logs           - Liste des logs")
    print("   GET  /api/logs/<id>      - Détails d'un log")
    print("   POST /api/logs           - Créer un log")
    print("   GET  /api/stats          - Statistiques")
    print("   GET  /api/health         - État du système")
    print("   POST /api/identify       - Identifier un utilisateur via empreinte")
    print()
    print(" Démarrage du serveur...")
    print("=" * 50)
    print()
    
    app.run(host='0.0.0.0', port=5000, debug=True)
