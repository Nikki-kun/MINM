from auth_manager import app
import time

if __name__ == '__main__':
    from auth_manager import users_db
    
    users_db['test'] = {
        'id': 1001,
        'username': 'test',
        'password': 'test123',
        'online': False,
        'lastSeen': int(time.time() * 1000),
        'contacts': [
            {
                'id': 2001,
                'ownerId': 1001,
                'contactId': 1002,
                'contactName': 'Друг 1',
                'addedDate': int(time.time() * 1000) - 86400000
            }
        ],
        'blockedUsers': []
    }
    
    users_db['admin'] = {
        'id': 1002,
        'username': 'admin',
        'password': 'admin123',
        'online': False,
        'lastSeen': int(time.time() * 1000),
        'contacts': [
            {
                'id': 2002,
                'ownerId': 1002,
                'contactId': 1001,
                'contactName': 'Тестовый пользователь',
                'addedDate': int(time.time() * 1000) - 43200000
            }
        ],
        'blockedUsers': []
    }
    
    app.run(host='0.0.0.0', port=8080, debug=True)