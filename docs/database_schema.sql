-- MINM Messenger - MySQL Schema
-- Соответствует структурам данных: Contact, Chat, Message, User

-- ============================================
-- Пользователи (User)
-- ============================================
CREATE TABLE users (
    id              INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username        VARCHAR(100) NOT NULL,
    password_hash   VARCHAR(255) NOT NULL,  -- хранить хэш, не пароль!
    online          BOOLEAN DEFAULT FALSE,
    last_seen       DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at      DATETIME DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================
-- Контакты (Contact)
-- ============================================
CREATE TABLE contacts (
    id              INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    owner_id        INT NOT NULL,
    contact_id      INT NOT NULL,
    contact_name    VARCHAR(100) NOT NULL,
    added_date      DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (contact_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY uk_owner_contact (owner_id, contact_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================
-- Заблокированные пользователи (User.blockedUsers)
-- ============================================
CREATE TABLE blocked_users (
    user_id         INT NOT NULL,
    blocked_user_id INT NOT NULL,
    blocked_at      DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, blocked_user_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (blocked_user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================
-- Чаты (Chat)
-- ============================================
CREATE TABLE chats (
    id              INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    type            TINYINT NOT NULL,  -- 0=PRIVATE, 1=GROUP, 2=CHANNEL
    created_date    DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Участники чата (Chat.participants - many-to-many)
CREATE TABLE chat_participants (
    chat_id         INT NOT NULL,
    user_id         INT NOT NULL,
    joined_at       DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (chat_id, user_id),
    FOREIGN KEY (chat_id) REFERENCES chats(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================
-- Сообщения (Message)
-- ============================================
CREATE TABLE messages (
    id              INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_id       INT NOT NULL,
    chat_id         INT NULL,           -- NULL = broadcast (MESSAGE_BROADCAST)
    content         VARCHAR(1000) NOT NULL,
    timestamp       DATETIME DEFAULT CURRENT_TIMESTAMP,
    status          TINYINT DEFAULT 0, -- 0=SENT, 1=DELIVERED, 2=READ, 3=FAILED
    type            TINYINT DEFAULT 0,  -- 0=MESSAGE_NORMAL, 1=MESSAGE_BROADCAST
    FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (chat_id) REFERENCES chats(id) ON DELETE SET NULL,
    INDEX idx_chat_timestamp (chat_id, timestamp),
    INDEX idx_sender (sender_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================
-- Индексы для частых запросов
-- ============================================
-- Контакты пользователя
CREATE INDEX idx_contacts_owner ON contacts(owner_id);

-- Чаты пользователя (через participants)
CREATE INDEX idx_chat_participants_user ON chat_participants(user_id);
