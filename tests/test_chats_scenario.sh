#!/bin/bash

PORT=${1:-8080}
SERVER_BINARY="./build/MINM"
BASE_URL="http://localhost:${PORT}"
DB_NAME="${MINM_DB_NAME:-minm_test}"
DB_USER="${MINM_DB_USER:-minm}"
DB_PASSWORD="${MINM_DB_PASSWORD:-minm_password}"
DB_ROOT_PASSWORD="${MINM_DB_ROOT_PASSWORD:-}"

MINM_DB_HOST="${MINM_DB_HOST:-127.0.0.1}"
MINM_DB_PORT="${MINM_DB_PORT:-3306}"
MINM_DB_RESET="${MINM_DB_RESET:-1}"

MYSQL_ROOT_ARGS="--protocol=socket -uroot"
if [ -n "$DB_ROOT_PASSWORD" ]; then
    MYSQL_ROOT_ARGS="$MYSQL_ROOT_ARGS -p${DB_ROOT_PASSWORD}"
fi

setup_mysql() {
    echo "MySQL: starting service (best-effort)..."
    sudo service mysql start >/dev/null 2>&1 || true
    sudo systemctl start mysql >/dev/null 2>&1 || true

    echo "MySQL: waiting for server..."
    local ok=0
    for i in $(seq 1 30); do
        if sudo mysql $MYSQL_ROOT_ARGS -e "SELECT 1;" >/dev/null 2>&1; then
            ok=1
            break
        fi
        sleep 1
    done
    if [ "$ok" -ne 1 ]; then
        echo "MySQL: failed to connect as root (check service/logs)."
        exit 1
    fi

    if [ "$MINM_DB_RESET" = "1" ]; then
        echo "MySQL: (re)creating database '$DB_NAME'..."
        sudo mysql $MYSQL_ROOT_ARGS -e "DROP DATABASE IF EXISTS \`${DB_NAME}\`; CREATE DATABASE \`${DB_NAME}\` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
    else
        echo "MySQL: ensuring database '$DB_NAME' exists..."
        sudo mysql $MYSQL_ROOT_ARGS -e "CREATE DATABASE IF NOT EXISTS \`${DB_NAME}\` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
    fi

    echo "MySQL: creating app user '$DB_USER'..."
    sudo mysql $MYSQL_ROOT_ARGS -e "DROP USER IF EXISTS '${DB_USER}'@'%';"
    sudo mysql $MYSQL_ROOT_ARGS -e "CREATE USER '${DB_USER}'@'%' IDENTIFIED BY '${DB_PASSWORD}';"
    sudo mysql $MYSQL_ROOT_ARGS -e "GRANT ALL PRIVILEGES ON \`${DB_NAME}\`.* TO '${DB_USER}'@'%'; FLUSH PRIVILEGES;"

    if [ "$MINM_DB_RESET" = "1" ]; then
        echo "MySQL: loading schema from docs/database_schema.sql..."
        sudo mysql $MYSQL_ROOT_ARGS "${DB_NAME}" < "./docs/database_schema.sql"
    else
        echo "MySQL: checking schema presence (required tables)..."
        local missingTables
        missingTables=$(sudo mysql $MYSQL_ROOT_ARGS "${DB_NAME}" -N -s -e "
            SELECT COUNT(*)
            FROM information_schema.tables
            WHERE table_schema='${DB_NAME}'
              AND table_name IN ('users','contacts','chats','chat_participants','messages')
        " || echo "0")

        if [ "$missingTables" -ne "5" ]; then
            echo "MySQL: some tables missing -> loading schema..."
            sudo mysql $MYSQL_ROOT_ARGS "${DB_NAME}" < "./docs/database_schema.sql"
        else
            echo "MySQL: schema already exists (all required tables present)."
        fi
    fi

    echo "MySQL: seeding users (idempotent, keep user_id=0)..."
    sudo mysql $MYSQL_ROOT_ARGS "${DB_NAME}" -e "
        SET SESSION sql_mode='NO_AUTO_VALUE_ON_ZERO';
        INSERT INTO users (user_id, username, password_hash, online) VALUES
            (0, 'user0', 'test_hash', 0),
            (1, 'user1', 'test_hash', 0),
            (2, 'user2', 'test_hash', 0),
            (3, 'user3', 'test_hash', 0),
            (4, 'user4', 'test_hash', 0),
            (5, 'user5', 'test_hash', 0)
        ON DUPLICATE KEY UPDATE
            username = VALUES(username),
            password_hash = VALUES(password_hash),
            online = VALUES(online);
    "
}

export MINM_DB_ENABLED=1
export MINM_DB_HOST="${MINM_DB_HOST}"
export MINM_DB_PORT="${MINM_DB_PORT}"
export MINM_DB_NAME="${DB_NAME}"
export MINM_DB_USER="${DB_USER}"
export MINM_DB_PASSWORD="${DB_PASSWORD}"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0

PRIVATE_CHAT_IDS=()
GROUP_CHAT_IDS=()
CHANNEL_CHAT_IDS=()

print_test() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
}

print_subtest() {
    echo -e "${CYAN}--- $1 ---${NC}"
}

check_result() {
    local test_name="$1"
    local response="$2"
    local expected_status="$3"
    
    if [ -z "$response" ]; then
        echo -e "${RED}✗ FAILED${NC}: $test_name (пустой ответ)"
        ((TESTS_FAILED++))
        return 1
    fi
    
    if echo "$response" | grep -q "$expected_status"; then
        echo -e "${GREEN}✓ PASSED${NC}: $test_name"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}✗ FAILED${NC}: $test_name"
        echo "Response: $response"
        ((TESTS_FAILED++))
        return 1
    fi
}

curl_request() {
    local method="$1"
    local endpoint="$2"
    local data="$3"
    
    if [ -z "$data" ]; then
        curl -s "${BASE_URL}${endpoint}"
    else
        curl -s -X "$method" "${BASE_URL}${endpoint}" \
            -H "Content-Type: application/json" \
            -d "$data"
    fi
}

print_json() {
    echo "$1" | python3 -m json.tool 2>/dev/null || echo "$1"
}

get_last_chat_id() {
    local response=$(curl_request "GET" "/chats")
    echo "$response" | python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    chats = data.get('chats', [])
    if chats:
        max_id = max((c.get('id', 0) for c in chats), default=0)
        print(max_id)
    else:
        print(0)
except Exception as e:
    print(0)
" 2>/dev/null || echo "0"
}

if [ ! -f "$SERVER_BINARY" ]; then
    echo -e "${RED}Ошибка: Бинарник $SERVER_BINARY не найден!${NC}"
    echo "Сначала соберите проект: make build"
    exit 1
fi

##setup_mysql

if ! command -v curl &> /dev/null; then
    echo -e "${RED}Ошибка: curl не установлен!${NC}"
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo -e "${YELLOW}Предупреждение: python3 не найден, JSON будет выводиться без форматирования${NC}"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Тестирование сценария с чатами${NC}"
echo -e "${GREEN}Порт: $PORT${NC}"
echo -e "${GREEN}========================================${NC}"

print_test "Запуск сервера"
echo "Запускаю сервер на порту $PORT..."
DISPLAY=:0 $SERVER_BINARY -p $PORT > /tmp/minm_server.log 2>&1 &
SERVER_PID=$!

echo "Ожидание запуска сервера..."
sleep 1

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}Ошибка: Сервер не запустился!${NC}"
    echo "Логи:"
    cat /tmp/minm_server.log
    exit 1
fi

echo "Проверка доступности сервера..."
MAX_RETRIES=10
RETRY_COUNT=0
while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if curl -s "${BASE_URL}/contacts" > /dev/null 2>&1; then
        echo -e "${GREEN}Сервер доступен (PID: $SERVER_PID)${NC}"
        break
    fi
    RETRY_COUNT=$((RETRY_COUNT + 1))
    sleep 1
done

if [ $RETRY_COUNT -eq $MAX_RETRIES ]; then
    echo -e "${RED}Ошибка: Сервер не отвечает на запросы!${NC}"
    echo "Логи:"
    tail -20 /tmp/minm_server.log
    kill $SERVER_PID 2>/dev/null || true
    exit 1
fi

cleanup() {
    if [ -n "$SERVER_PID" ] && kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "\n${YELLOW}Остановка сервера...${NC}"
        kill $SERVER_PID 2>/dev/null || true
        sleep 1
        if kill -0 $SERVER_PID 2>/dev/null; then
            kill -9 $SERVER_PID 2>/dev/null || true
        fi
        wait $SERVER_PID 2>/dev/null || true
        echo -e "${GREEN}Сервер остановлен${NC}"
    fi
}

trap cleanup EXIT

print_test "ЭТАП 1: Подготовка - создание контактов"

print_subtest "Создание контактов для тестирования (владелец — пользователь 0)"
RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 0, "contactId": 2, "contactName": "Алиса"}')
print_json "$RESPONSE"
check_result "Создание контакта Алиса" "$RESPONSE" '"status": "success"'

RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 0, "contactId": 3, "contactName": "Боб"}')
print_json "$RESPONSE"
check_result "Создание контакта Боб" "$RESPONSE" '"status": "success"'

RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 0, "contactId": 4, "contactName": "Чарли"}')
print_json "$RESPONSE"
check_result "Создание контакта Чарли" "$RESPONSE" '"status": "success"'

RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 0, "contactId": 5, "contactName": "Диана"}')
print_json "$RESPONSE"
check_result "Создание контакта Диана" "$RESPONSE" '"status": "success"'

print_test "ЭТАП 2: Создание приватных чатов (PRIVATE = 0)"

print_subtest "Приватный чат между пользователем 0 и Алисой (ID: 2)"
RESPONSE=$(curl_request "POST" "/chats" '{"type": 0, "participants": [0, 2]}')
print_json "$RESPONSE"
check_result "Создание приватного чата 0-2" "$RESPONSE" '"status": "success"'
sleep 0.3
CHAT_ID=$(get_last_chat_id)
if [ -n "$CHAT_ID" ] && [ "$CHAT_ID" != "0" ]; then
    PRIVATE_CHAT_IDS+=($CHAT_ID)
    echo -e "${GREEN}Создан приватный чат ID: $CHAT_ID${NC}"
fi

print_subtest "Приватный чат между пользователем 0 и Бобом (ID: 3)"
RESPONSE=$(curl_request "POST" "/chats" '{"type": 0, "participants": [0, 3]}')
print_json "$RESPONSE"
check_result "Создание приватного чата 0-3" "$RESPONSE" '"status": "success"'
sleep 0.3
CHAT_ID=$(get_last_chat_id)
if [ -n "$CHAT_ID" ] && [ "$CHAT_ID" != "0" ]; then
    PRIVATE_CHAT_IDS+=($CHAT_ID)
    echo -e "${GREEN}Создан приватный чат ID: $CHAT_ID${NC}"
fi

print_test "ЭТАП 3: Создание групповых чатов (GROUP = 1)"

print_subtest "Групповой чат 'Работа' с участниками 0, 2, 3"
RESPONSE=$(curl_request "POST" "/chats" '{"type": 1, "participants": [0, 2, 3]}')
print_json "$RESPONSE"
check_result "Создание группового чата 'Работа'" "$RESPONSE" '"status": "success"'
sleep 0.3
CHAT_ID=$(get_last_chat_id)
if [ -n "$CHAT_ID" ] && [ "$CHAT_ID" != "0" ]; then
    GROUP_CHAT_IDS+=($CHAT_ID)
    echo -e "${GREEN}Создан групповой чат ID: $CHAT_ID${NC}"
fi

print_subtest "Групповой чат 'Друзья' с участниками 0, 2, 3, 4, 5"
RESPONSE=$(curl_request "POST" "/chats" '{"type": 1, "participants": [0, 2, 3, 4, 5]}')
print_json "$RESPONSE"
check_result "Создание группового чата 'Друзья'" "$RESPONSE" '"status": "success"'
sleep 0.3
CHAT_ID=$(get_last_chat_id)
if [ -n "$CHAT_ID" ] && [ "$CHAT_ID" != "0" ]; then
    GROUP_CHAT_IDS+=($CHAT_ID)
    echo -e "${GREEN}Создан групповой чат ID: $CHAT_ID${NC}"
fi

print_test "ЭТАП 4: Создание каналов (CHANNEL = 2)"

print_subtest "Канал 'Новости' с участниками 0, 2, 3, 4, 5"
RESPONSE=$(curl_request "POST" "/chats" '{"type": 2, "participants": [0, 2, 3, 4, 5]}')
print_json "$RESPONSE"
check_result "Создание канала 'Новости'" "$RESPONSE" '"status": "success"'
sleep 0.3
CHAT_ID=$(get_last_chat_id)
if [ -n "$CHAT_ID" ] && [ "$CHAT_ID" != "0" ]; then
    CHANNEL_CHAT_IDS+=($CHAT_ID)
    echo -e "${GREEN}Создан канал ID: $CHAT_ID${NC}"
fi

print_subtest "Канал 'Объявления' с участниками 0, 2, 3"
RESPONSE=$(curl_request "POST" "/chats" '{"type": 2, "participants": [0, 2, 3]}')
print_json "$RESPONSE"
check_result "Создание канала 'Объявления'" "$RESPONSE" '"status": "success"'
sleep 0.3
CHAT_ID=$(get_last_chat_id)
if [ -n "$CHAT_ID" ] && [ "$CHAT_ID" != "0" ]; then
    CHANNEL_CHAT_IDS+=($CHAT_ID)
    echo -e "${GREEN}Создан канал ID: $CHAT_ID${NC}"
fi

print_test "ЭТАП 5: Отправка сообщений в приватные чаты"

if [ ${#PRIVATE_CHAT_IDS[@]} -gt 0 ]; then
    CHAT_ID=${PRIVATE_CHAT_IDS[0]}
    print_subtest "Сообщения в приватный чат ID: $CHAT_ID"
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 1, \"receiver_id\": $CHAT_ID, \"content\": \"Привет, Алиса! Как дела?\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 1 в приватный чат" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 2, \"receiver_id\": $CHAT_ID, \"content\": \"Привет! Всё отлично, спасибо! А у тебя?\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 2 в приватный чат" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 1, \"receiver_id\": $CHAT_ID, \"content\": \"Тоже хорошо! Хочешь встретиться на выходных?\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 3 в приватный чат" "$RESPONSE" '"status": "success"'
fi

if [ ${#PRIVATE_CHAT_IDS[@]} -gt 1 ]; then
    CHAT_ID=${PRIVATE_CHAT_IDS[1]}
    print_subtest "Сообщения в приватный чат ID: $CHAT_ID"
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 1, \"receiver_id\": $CHAT_ID, \"content\": \"Привет, Боб! Есть минутка?\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 1 в приватный чат 2" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 3, \"receiver_id\": $CHAT_ID, \"content\": \"Да, конечно! Что случилось?\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 2 в приватный чат 2" "$RESPONSE" '"status": "success"'
fi

print_test "ЭТАП 6: Отправка сообщений в групповые чаты"

if [ ${#GROUP_CHAT_IDS[@]} -gt 0 ]; then
    CHAT_ID=${GROUP_CHAT_IDS[0]}
    print_subtest "Сообщения в групповой чат 'Работа' ID: $CHAT_ID"
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 1, \"receiver_id\": $CHAT_ID, \"content\": \"Всем привет! Напоминаю о встрече завтра в 10:00\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 1 в групповой чат" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 2, \"receiver_id\": $CHAT_ID, \"content\": \"Понял, буду!\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 2 в групповой чат" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 3, \"receiver_id\": $CHAT_ID, \"content\": \"Я тоже приду. Подготовлю презентацию.\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 3 в групповой чат" "$RESPONSE" '"status": "success"'
fi

if [ ${#GROUP_CHAT_IDS[@]} -gt 1 ]; then
    CHAT_ID=${GROUP_CHAT_IDS[1]}
    print_subtest "Сообщения в групповой чат 'Друзья' ID: $CHAT_ID"
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 1, \"receiver_id\": $CHAT_ID, \"content\": \"Ребята, кто идёт на концерт в субботу?\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 1 в групповой чат 'Друзья'" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 2, \"receiver_id\": $CHAT_ID, \"content\": \"Я иду! Билеты уже купил.\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 2 в групповой чат 'Друзья'" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 4, \"receiver_id\": $CHAT_ID, \"content\": \"А я не смогу, у меня другие планы :(\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 3 в групповой чат 'Друзья'" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 5, \"receiver_id\": $CHAT_ID, \"content\": \"Я тоже иду! Встретимся там?\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 4 в групповой чат 'Друзья'" "$RESPONSE" '"status": "success"'
fi

print_test "ЭТАП 7: Отправка сообщений в каналы"

if [ ${#CHANNEL_CHAT_IDS[@]} -gt 0 ]; then
    CHAT_ID=${CHANNEL_CHAT_IDS[0]}
    print_subtest "Сообщения в канал 'Новости' ID: $CHAT_ID"
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 1, \"receiver_id\": $CHAT_ID, \"content\": \"📢 Важное объявление: Завтра будет обновление системы. Работа будет недоступна с 2:00 до 4:00.\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 1 в канал" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 1, \"receiver_id\": $CHAT_ID, \"content\": \"📅 Напоминание: Не забудьте заполнить отчёты до конца недели!\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 2 в канал" "$RESPONSE" '"status": "success"'
fi

if [ ${#CHANNEL_CHAT_IDS[@]} -gt 1 ]; then
    CHAT_ID=${CHANNEL_CHAT_IDS[1]}
    print_subtest "Сообщения в канал 'Объявления' ID: $CHAT_ID"
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 1, \"receiver_id\": $CHAT_ID, \"content\": \"🏢 Объявление: В офисе появилась новая кофемашина. Приходите попробовать!\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 1 в канал 'Объявления'" "$RESPONSE" '"status": "success"'
    
    sleep 0.5
    
    RESPONSE=$(curl_request "POST" "/messages" "{\"sender_id\": 2, \"receiver_id\": $CHAT_ID, \"content\": \"🎉 Отличная новость! Спасибо за обновление!\"}")
    print_json "$RESPONSE"
    check_result "Сообщение 2 в канал 'Объявления'" "$RESPONSE" '"status": "success"'
fi

print_test "ЭТАП 7.1: Рассылка сообщения во все чаты (type: 1 = BROADCAST)"

RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 0, "type": 1, "content": "📢 Важное объявление для всех: Завтра технические работы с 10:00 до 12:00. Пожалуйста, сохраните данные!"}')
print_json "$RESPONSE"
check_result "Рассылка во все чаты" "$RESPONSE" '"status": "success"'

print_test "ЭТАП 8: Итоговое состояние всех данных"

echo -e "\n${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${MAGENTA}📋 КОНТАКТЫ${NC}"
echo -e "${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
RESPONSE=$(curl_request "GET" "/contacts")
print_json "$RESPONSE"
CONTACT_COUNT=$(echo "$RESPONSE" | python3 -c "import sys, json; data=json.load(sys.stdin); print(data.get('count', 0))" 2>/dev/null || echo "?")
echo -e "${GREEN}Всего контактов: $CONTACT_COUNT${NC}"

echo -e "\n${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${MAGENTA}💬 ЧАТЫ${NC}"
echo -e "${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
RESPONSE=$(curl_request "GET" "/chats")
print_json "$RESPONSE"
CHAT_COUNT=$(echo "$RESPONSE" | python3 -c "import sys, json; data=json.load(sys.stdin); print(data.get('count', 0))" 2>/dev/null || echo "?")
echo -e "${GREEN}Всего чатов: $CHAT_COUNT${NC}"

PRIVATE_COUNT=$(echo "$RESPONSE" | python3 -c "import sys, json; data=json.load(sys.stdin); chats=data.get('chats', []); print(sum(1 for c in chats if c.get('type') == 0))" 2>/dev/null || echo "?")
GROUP_COUNT=$(echo "$RESPONSE" | python3 -c "import sys, json; data=json.load(sys.stdin); chats=data.get('chats', []); print(sum(1 for c in chats if c.get('type') == 1))" 2>/dev/null || echo "?")
CHANNEL_COUNT=$(echo "$RESPONSE" | python3 -c "import sys, json; data=json.load(sys.stdin); chats=data.get('chats', []); print(sum(1 for c in chats if c.get('type') == 2))" 2>/dev/null || echo "?")

echo -e "${CYAN}  🔒 Приватных чатов: $PRIVATE_COUNT${NC}"
echo -e "${CYAN}  👥 Групповых чатов: $GROUP_COUNT${NC}"
echo -e "${CYAN}  📢 Каналов: $CHANNEL_COUNT${NC}"

echo -e "\n${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${MAGENTA}📨 СООБЩЕНИЯ${NC}"
echo -e "${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
RESPONSE=$(curl_request "GET" "/messages")
print_json "$RESPONSE"
MESSAGE_COUNT=$(echo "$RESPONSE" | python3 -c "import sys, json; data=json.load(sys.stdin); print(data.get('count', 0))" 2>/dev/null || echo "?")
echo -e "${GREEN}Всего сообщений: $MESSAGE_COUNT${NC}"

echo -e "\n${YELLOW}Распределение сообщений по чатам:${NC}"
if [ -n "$RESPONSE" ]; then
    echo "$RESPONSE" | python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    messages = data.get('messages', [])
    chat_counts = {}
    for msg in messages:
        chat_id = msg.get('receiver_id', 0)
        chat_counts[chat_id] = chat_counts.get(chat_id, 0) + 1
    for chat_id, count in sorted(chat_counts.items()):
        print(f'  Чат ID {chat_id}: {count} сообщений')
except:
    pass
" 2>/dev/null || echo "  Не удалось подсчитать"
fi

echo -e "\n${GREEN}========================================${NC}"
echo -e "${GREEN}📊 ИТОГОВАЯ СТАТИСТИКА${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Пройдено тестов: $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Провалено тестов: $TESTS_FAILED${NC}"
else
    echo -e "${GREEN}Провалено тестов: 0${NC}"
fi
echo -e "\n${CYAN}Созданные ресурсы:${NC}"
echo -e "  📇 Контакты: $CONTACT_COUNT"
echo -e "  💬 Чаты: $CHAT_COUNT"
echo -e "    🔒 Приватных: $PRIVATE_COUNT"
echo -e "    👥 Групповых: $GROUP_COUNT"
echo -e "    📢 Каналов: $CHANNEL_COUNT"
echo -e "  📨 Сообщения: $MESSAGE_COUNT"
echo -e "${GREEN}========================================${NC}"

trap - EXIT

echo -e "\n${YELLOW}========================================${NC}"
echo -e "${YELLOW}Сервер продолжает работать${NC}"
echo -e "${YELLOW}========================================${NC}"
echo -e "Сервер доступен по адресу: ${GREEN}${BASE_URL}${NC}"
echo -e "PID сервера: ${GREEN}${SERVER_PID}${NC}"
echo -e "\n${YELLOW}Нажмите Enter для остановки сервера...${NC}"

read -r

cleanup

if [ $TESTS_FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
