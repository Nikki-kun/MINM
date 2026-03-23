# Архитектура базы данных MINM

## Обзор

Документ описывает соответствие структур данных C++ и схемы MySQL, а также рекомендации по интеграции.

## Соответствие структур

| C++ класс | MySQL таблица | Примечания |
|-----------|---------------|------------|
| User | users | Пароль → password_hash (хранение хэша) |
| Contact | contacts | 1:1 соответствие полей |
| User.blockedUsers | blocked_users | Отдельная таблица many-to-many |
| Chat | chats + chat_participants | participants вынесены в отдельную таблицу |
| Message | messages | receiver_id=-1 → chat_id=NULL для broadcast |

## Рекомендации по структурам данных

### 1. Contact — готов к MySQL ✓

Текущая структура подходит. Рекомендации:
- Добавить уникальный индекс `(owner_id, contact_id)` — уже в схеме
- `addedDate` (QDateTime) ↔ `DATETIME` — прямая конвертация

### 2. Chat — нормализация для MySQL

**Текущее:** `participants` и `messages` — векторы внутри объекта.

**В MySQL:**
- `chats` — только `chat_id`, `type`, `chat_created_at`
- `chat_participants` — связь many-to-many
- `messages.chat_id` — связь сообщений с чатом (без отдельной таблицы chat_messages)

**Рекомендация:** В C++ оставить текущую структуру для in-memory кэша. При загрузке из БД — заполнять `participants` из `chat_participants`, `messages` — запросом `SELECT message_id FROM messages WHERE chat_id=? ORDER BY message_created_at`.

### 3. Message — небольшое уточнение

**Текущее:** `receiver_id` = chat_id, -1 для broadcast.

**В MySQL:** `chat_id` NULL для broadcast (семантически корректнее).

**Рекомендация:** При маппинге:
- C++ → DB: `receiver_id == -1` → `chat_id = NULL`
- DB → C++: `chat_id IS NULL` → `receiver_id = -1`

### 4. User — интеграция в основной поток

Класс `User` есть, но не используется в `main()` и `MessageManager`. Для MySQL нужна таблица `users`.

**Рекомендации:**
- Подключить `AuthManager` для аутентификации
- Хранить `password_hash`, не plain password
- `User.contacts` — дублирует `contacts` по owner_id; в БД использовать только таблицу `contacts`

### 5. Типы данных — конвертация

| C++ | MySQL |
|-----|-------|
| `std::chrono::system_clock::time_point` | `DATETIME` |
| `QDateTime` | `DATETIME` |
| `int32_t` (id) | `INT` |
| `message_status` enum | `TINYINT` (0-3) |
| `chat_type` enum | `TINYINT` (0-2) |
| `message_type` enum | `TINYINT` (0-1) |

## Слои для интеграции MySQL

```
┌─────────────────┐
│  MessageManager │  ← бизнес-логика (без изменений)
├─────────────────┤
│  DataRepository │  ← НОВЫЙ: абстракция доступа к данным
│  (interface)    │     - loadContacts(), saveContact(), ...
├─────────────────┤
│  InMemoryRepo   │  ← текущая реализация (QVector)
│  MySQLRepo      │  ← новая: MySQL через connector
└─────────────────┘
```

**Стратегия:** Ввести интерфейс `IDataRepository` с методами CRUD. `MessageManager` получает репозиторий через конструктор. Реализации: `InMemoryRepository` (текущая логика) и `MySQLRepository`.

## Порядок миграции

1. Создать БД по `docs/database_schema.sql`
2. Добавить `IDataRepository` и `MySQLRepository`
3. Подключить Qt MySQL driver или mysql-connector-cpp
4. Переключить `MessageManager` на `MySQLRepository` через конфиг/флаг
5. Реализовать `AuthManager` с проверкой против `users`

## Ограничения (из types.h)

- `MAX_USERNAME_LENGTH = 100` → `VARCHAR(100)` ✓
- `MAX_CONTACT_NAME_LENGTH = 100` → `VARCHAR(100)` ✓
- `MAX_MESSAGE_LENGTH = 1000` → `VARCHAR(1000)` ✓
