from flask import Flask, request, jsonify
from datetime import datetime
import time
import random

app = Flask(__name__)

users_db = {}
contacts_db = {}
sessions = {}

def generate_id():
    return random.randint(1000, 9999)

@app.route('/api/auth/register', methods=['POST'])
def register():
    data = request.get_json()
    username = data.get('username')
    password = data.get('password')
    
    if not username or not password:
        return jsonify({
            'success': False,
            'error': 'Username and password are required'
        }), 400
    
    if username in users_db:
        return jsonify({
            'success': False,
            'error': 'User already exists'
        }), 400
    
    user_id = generate_id()
    users_db[username] = {
        'id': user_id,
        'username': username,
        'password': password,
        'online': False,
        'lastSeen': int(time.time() * 1000),
        'contacts': [],
        'blockedUsers': []
    }
    
    token = f"token_{user_id}_{int(time.time())}"
    sessions[token] = user_id
    
    return jsonify({
        'success': True,
        'token': token,
        'user': users_db[username]
    })

@app.route('/api/auth/login', methods=['POST'])
def login():
    data = request.get_json()
    username = data.get('username')
    password = data.get('password')
    
    if not username or not password:
        return jsonify({
            'success': False,
            'error': 'Username and password are required'
        }), 400
    
    user = users_db.get(username)
    if not user or user['password'] != password:
        return jsonify({
            'success': False,
            'error': 'Invalid username or password'
        }), 401
    
    user['online'] = True
    user['lastSeen'] = int(time.time() * 1000)
    
    token = f"token_{user['id']}_{int(time.time())}"
    sessions[token] = user['id']
    
    return jsonify({
        'success': True,
        'token': token,
        'user': user
    })

@app.route('/api/auth/logout', methods=['POST'])
def logout():
    token = request.headers.get('Authorization', '').replace('Bearer ', '')
    
    if not token or token not in sessions:
        return jsonify({
            'success': False,
            'error': 'Invalid token'
        }), 401
    
    user_id = sessions[token]
    
    for user in users_db.values():
        if user['id'] == user_id:
            user['online'] = False
            user['lastSeen'] = int(time.time() * 1000)
            break
    
    del sessions[token]
    
    return jsonify({
        'success': True,
        'message': 'Logged out successfully'
    })

@app.route('/api/user/contacts', methods=['GET'])
def get_contacts():
    token = request.headers.get('Authorization', '').replace('Bearer ', '')
    
    if not token or token not in sessions:
        return jsonify({
            'success': False,
            'error': 'Invalid token'
        }), 401
    
    user_id = sessions[token]
    
    user = None
    for u in users_db.values():
        if u['id'] == user_id:
            user = u
            break
    
    if not user:
        return jsonify({
            'success': False,
            'error': 'User not found'
        }), 404
    
    return jsonify({
        'success': True,
        'contacts': user['contacts']
    })

@app.route('/api/user/profile', methods=['GET'])
def get_profile():
    token = request.headers.get('Authorization', '').replace('Bearer ', '')
    
    if not token or token not in sessions:
        return jsonify({
            'success': False,
            'error': 'Invalid token'
        }), 401
    
    user_id = sessions[token]
    
    user = None
    for u in users_db.values():
        if u['id'] == user_id:
            user = u
            break
    
    if not user:
        return jsonify({
            'success': False,
            'error': 'User not found'
        }), 404
    
    return jsonify({
        'success': True,
        'user': user
    })

@app.route('/api/debug/users', methods=['GET'])
def debug_users():
    return jsonify({
        'users': users_db,
        'sessions': sessions
    })

@app.route('/api/health', methods=['GET'])
def health():
    return jsonify({
        'status': 'OK',
        'timestamp': datetime.now().isoformat(),
        'users_count': len(users_db),
        'sessions_count': len(sessions)
    })