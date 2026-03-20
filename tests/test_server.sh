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

        # We expect exactly 5 required tables.
        if [ "$missingTables" -ne "5" ]; then
            echo "MySQL: some tables missing -> loading schema..."
            sudo mysql $MYSQL_ROOT_ARGS "${DB_NAME}" < "./docs/database_schema.sql"
        else
            echo "MySQL: schema already exists (all required tables present)."
        fi
    fi

    echo "MySQL: seeding users (idempotent, keep id=0)..."
    sudo mysql $MYSQL_ROOT_ARGS "${DB_NAME}" -e "
        SET SESSION sql_mode='NO_AUTO_VALUE_ON_ZERO';
        INSERT INTO users (id, username, password_hash, online) VALUES
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
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0

print_test() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
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

if [ ! -f "$SERVER_BINARY" ]; then
    echo -e "${RED}Ошибка: Бинарник $SERVER_BINARY не найден!${NC}"
    echo "Сначала соберите проект: cd build && cmake .. && make"
    exit 1
fi

setup_mysql

if ! command -v curl &> /dev/null; then
    echo -e "${RED}Ошибка: curl не установлен!${NC}"
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo -e "${YELLOW}Предупреждение: python3 не найден, JSON будет выводиться без форматирования${NC}"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Тестирование HTTP сервера чата${NC}"
echo -e "${GREEN}Порт: $PORT${NC}"
echo -e "${GREEN}========================================${NC}"

print_test "Запуск сервера"
echo "Запускаю сервер на порту $PORT..."
DISPLAY=:0 $SERVER_BINARY -p $PORT > /tmp/minm_server.log 2>&1 &
SERVER_PID=$!

echo "Ожидание запуска сервера..."
sleep 3

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
        # Принудительное завершение, если не остановился
        if kill -0 $SERVER_PID 2>/dev/null; then
            kill -9 $SERVER_PID 2>/dev/null || true
        fi
        wait $SERVER_PID 2>/dev/null || true
        echo -e "${GREEN}Сервер остановлен${NC}"
    fi
}

trap cleanup EXIT

print_test "Тест 1: GET /contacts (начальное состояние)"
RESPONSE=$(curl_request "GET" "/contacts")
print_json "$RESPONSE"
check_result "GET /contacts (пустой)" "$RESPONSE" '"count": 0'

print_test "Тест 2: POST /contacts (добавление контакта)"
RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 1, "contactId": 2, "contactName": "Иван Иванов"}')
print_json "$RESPONSE"
check_result "POST /contacts" "$RESPONSE" '"status": "success"'

print_test "Тест 3: GET /contacts (после добавления)"
RESPONSE=$(curl_request "GET" "/contacts")
print_json "$RESPONSE"
check_result "GET /contacts (с данными)" "$RESPONSE" '"count": 1'

print_test "Тест 4: POST /contacts (добавление второго контакта)"
RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 1, "contactId": 3, "contactName": "Мария Петрова"}')
print_json "$RESPONSE"
check_result "POST /contacts (второй)" "$RESPONSE" '"status": "success"'

print_test "Тест 5: GET /chats (начальное состояние)"
RESPONSE=$(curl_request "GET" "/chats")
print_json "$RESPONSE"
check_result "GET /chats (пустой)" "$RESPONSE" '"count": 0'

print_test "Тест 6: POST /chats (создание чата)"
RESPONSE=$(curl_request "POST" "/chats" '{"type": 0, "participants": [1, 2]}')
print_json "$RESPONSE"
check_result "POST /chats" "$RESPONSE" '"status": "success"'

print_test "Тест 7: GET /chats (после создания)"
RESPONSE=$(curl_request "GET" "/chats")
print_json "$RESPONSE"
check_result "GET /chats (с данными)" "$RESPONSE" '"count": 1'

print_test "Тест 8: GET /messages (начальное состояние)"
RESPONSE=$(curl_request "GET" "/messages")
print_json "$RESPONSE"
check_result "GET /messages (пустой)" "$RESPONSE" '"count": 0'

print_test "Тест 9: POST /messages (отправка сообщения)"
RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 1, "receiver_id": 1, "content": "Привет! Как дела?"}')
print_json "$RESPONSE"
check_result "POST /messages" "$RESPONSE" '"status": "success"'

print_test "Тест 10: POST /messages (второе сообщение)"
RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 1, "receiver_id": 1, "content": "Второе сообщение"}')
print_json "$RESPONSE"
check_result "POST /messages (второе)" "$RESPONSE" '"status": "success"'

print_test "Тест 11: POST /messages (третье сообщение)"
RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 2, "receiver_id": 1, "content": "Ответ от Ивана"}')
print_json "$RESPONSE"
check_result "POST /messages (третье)" "$RESPONSE" '"status": "success"'

print_test "Тест 12: GET /messages (все сообщения)"
RESPONSE=$(curl_request "GET" "/messages")
print_json "$RESPONSE"
check_result "GET /messages (с данными)" "$RESPONSE" '"count": 3'

print_test "Тест 13: POST /contacts с неполными данными (проверка валидации)"
RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 1}')
print_json "$RESPONSE"
check_result "POST /contacts (валидация)" "$RESPONSE" '"status": "error"'

print_test "Тест 14: POST /messages с неполными данными (проверка валидации)"
RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 1}')
print_json "$RESPONSE"
check_result "POST /messages (валидация)" "$RESPONSE" '"status": "error"'

print_test "Тест 15: GET /unknown (неизвестный эндпоинт)"
RESPONSE=$(curl_request "GET" "/unknown")
print_json "$RESPONSE"
check_result "GET /unknown" "$RESPONSE" '"error"'

print_test "Тест 16: Финальное состояние всех данных"
echo -e "${YELLOW}Контакты:${NC}"
RESPONSE=$(curl_request "GET" "/contacts")
print_json "$RESPONSE"
echo -e "\n${YELLOW}Чаты:${NC}"
RESPONSE=$(curl_request "GET" "/chats")
print_json "$RESPONSE"
echo -e "\n${YELLOW}Сообщения:${NC}"
RESPONSE=$(curl_request "GET" "/messages")
print_json "$RESPONSE"

echo -e "\n${GREEN}========================================${NC}"
echo -e "${GREEN}Результаты тестирования:${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Пройдено тестов: $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Провалено тестов: $TESTS_FAILED${NC}"
else
    echo -e "${GREEN}Провалено тестов: 0${NC}"
fi
echo -e "${GREEN}========================================${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}Все тесты пройдены успешно!${NC}"
else
    echo -e "${RED}Некоторые тесты провалены!${NC}"
fi

trap - EXIT

echo -e "\n${YELLOW}========================================${NC}"
echo -e "${YELLOW}Сервер продолжает работать${NC}"
echo -e "${YELLOW}========================================${NC}"
echo -e "Сервер доступен по адресу: ${GREEN}${BASE_URL}${NC}"
echo -e "PID сервера: ${GREEN}${SERVER_PID}${NC}"
echo -e "\n${YELLOW}Доступные эндпоинты:${NC}"
echo -e "  GET  ${BASE_URL}/contacts"
echo -e "  POST ${BASE_URL}/contacts"
echo -e "  GET  ${BASE_URL}/chats"
echo -e "  POST ${BASE_URL}/chats"
echo -e "  GET  ${BASE_URL}/messages"
echo -e "  POST ${BASE_URL}/messages"
echo -e "\n${YELLOW}Нажмите Enter для остановки сервера...${NC}"

read -r

cleanup

if [ $TESTS_FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
