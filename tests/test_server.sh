#!/bin/bash

# Скрипт для тестирования HTTP сервера чата
# Использование: ./test_server.sh [port]

PORT=${1:-8080}
SERVER_BINARY="./build/MINM"
BASE_URL="http://localhost:${PORT}"

# Цвета для вывода
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Счетчики
TESTS_PASSED=0
TESTS_FAILED=0

# Функция для вывода заголовка теста
print_test() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
}

# Функция для проверки успешности теста
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

# Функция для выполнения curl запроса
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

# Функция для форматированного вывода JSON
print_json() {
    echo "$1" | python3 -m json.tool 2>/dev/null || echo "$1"
}

# Проверка наличия бинарника
if [ ! -f "$SERVER_BINARY" ]; then
    echo -e "${RED}Ошибка: Бинарник $SERVER_BINARY не найден!${NC}"
    echo "Сначала соберите проект: cd build && cmake .. && make"
    exit 1
fi

# Проверка наличия curl
if ! command -v curl &> /dev/null; then
    echo -e "${RED}Ошибка: curl не установлен!${NC}"
    exit 1
fi

# Проверка наличия python3
if ! command -v python3 &> /dev/null; then
    echo -e "${YELLOW}Предупреждение: python3 не найден, JSON будет выводиться без форматирования${NC}"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Тестирование HTTP сервера чата${NC}"
echo -e "${GREEN}Порт: $PORT${NC}"
echo -e "${GREEN}========================================${NC}"

# Запуск сервера
print_test "Запуск сервера"
echo "Запускаю сервер на порту $PORT..."
DISPLAY=:0 $SERVER_BINARY -p $PORT > /tmp/minm_server.log 2>&1 &
SERVER_PID=$!

# Ожидание запуска сервера
echo "Ожидание запуска сервера..."
sleep 3

# Проверка, что сервер запустился
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}Ошибка: Сервер не запустился!${NC}"
    echo "Логи:"
    cat /tmp/minm_server.log
    exit 1
fi

# Проверка доступности сервера
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

# Функция очистки при выходе
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

# Тест 1: GET /contacts (пустой список)
print_test "Тест 1: GET /contacts (начальное состояние)"
RESPONSE=$(curl_request "GET" "/contacts")
print_json "$RESPONSE"
check_result "GET /contacts (пустой)" "$RESPONSE" '"count": 0'

# Тест 2: POST /contacts (добавление первого контакта)
print_test "Тест 2: POST /contacts (добавление контакта)"
RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 1, "contactId": 2, "contactName": "Иван Иванов"}')
print_json "$RESPONSE"
check_result "POST /contacts" "$RESPONSE" '"status": "success"'

# Тест 3: GET /contacts (после добавления)
print_test "Тест 3: GET /contacts (после добавления)"
RESPONSE=$(curl_request "GET" "/contacts")
print_json "$RESPONSE"
check_result "GET /contacts (с данными)" "$RESPONSE" '"count": 1'

# Тест 4: POST /contacts (добавление второго контакта)
print_test "Тест 4: POST /contacts (добавление второго контакта)"
RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 1, "contactId": 3, "contactName": "Мария Петрова"}')
print_json "$RESPONSE"
check_result "POST /contacts (второй)" "$RESPONSE" '"status": "success"'

# Тест 5: GET /chats (пустой список)
print_test "Тест 5: GET /chats (начальное состояние)"
RESPONSE=$(curl_request "GET" "/chats")
print_json "$RESPONSE"
check_result "GET /chats (пустой)" "$RESPONSE" '"count": 0'

# Тест 6: POST /chats (создание чата)
print_test "Тест 6: POST /chats (создание чата)"
RESPONSE=$(curl_request "POST" "/chats" '{"type": 0, "participants": [1, 2]}')
print_json "$RESPONSE"
check_result "POST /chats" "$RESPONSE" '"status": "success"'

# Тест 7: GET /chats (после создания)
print_test "Тест 7: GET /chats (после создания)"
RESPONSE=$(curl_request "GET" "/chats")
print_json "$RESPONSE"
check_result "GET /chats (с данными)" "$RESPONSE" '"count": 1'

# Тест 8: GET /messages (пустой список)
print_test "Тест 8: GET /messages (начальное состояние)"
RESPONSE=$(curl_request "GET" "/messages")
print_json "$RESPONSE"
check_result "GET /messages (пустой)" "$RESPONSE" '"count": 0'

# Тест 9: POST /messages (отправка первого сообщения)
print_test "Тест 9: POST /messages (отправка сообщения)"
RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 1, "receiver_id": 1, "content": "Привет! Как дела?"}')
print_json "$RESPONSE"
check_result "POST /messages" "$RESPONSE" '"status": "success"'

# Тест 10: POST /messages (отправка второго сообщения)
print_test "Тест 10: POST /messages (второе сообщение)"
RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 1, "receiver_id": 1, "content": "Второе сообщение"}')
print_json "$RESPONSE"
check_result "POST /messages (второе)" "$RESPONSE" '"status": "success"'

# Тест 11: POST /messages (отправка третьего сообщения)
print_test "Тест 11: POST /messages (третье сообщение)"
RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 2, "receiver_id": 1, "content": "Ответ от Ивана"}')
print_json "$RESPONSE"
check_result "POST /messages (третье)" "$RESPONSE" '"status": "success"'

# Тест 12: GET /messages (получение всех сообщений)
print_test "Тест 12: GET /messages (все сообщения)"
RESPONSE=$(curl_request "GET" "/messages")
print_json "$RESPONSE"
check_result "GET /messages (с данными)" "$RESPONSE" '"count": 3'

# Тест 13: POST /contacts с неполными данными (ошибка)
print_test "Тест 13: POST /contacts с неполными данными (проверка валидации)"
RESPONSE=$(curl_request "POST" "/contacts" '{"ownerId": 1}')
print_json "$RESPONSE"
check_result "POST /contacts (валидация)" "$RESPONSE" '"status": "error"'

# Тест 14: POST /messages с неполными данными (ошибка)
print_test "Тест 14: POST /messages с неполными данными (проверка валидации)"
RESPONSE=$(curl_request "POST" "/messages" '{"sender_id": 1}')
print_json "$RESPONSE"
check_result "POST /messages (валидация)" "$RESPONSE" '"status": "error"'

# Тест 15: Неизвестный эндпоинт
print_test "Тест 15: GET /unknown (неизвестный эндпоинт)"
RESPONSE=$(curl_request "GET" "/unknown")
print_json "$RESPONSE"
check_result "GET /unknown" "$RESPONSE" '"error"'

# Тест 16: Финальное состояние всех данных
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

# Итоговая статистика
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

# Возвращаем код выхода в зависимости от результатов
if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}Все тесты пройдены успешно!${NC}"
else
    echo -e "${RED}Некоторые тесты провалены!${NC}"
fi

# Отключаем автоматическую очистку при EXIT
trap - EXIT

# Информация о сервере
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

# Ожидание ввода пользователя
read -r

# Остановка сервера после ввода
cleanup

# Возвращаем код выхода в зависимости от результатов тестов
if [ $TESTS_FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
