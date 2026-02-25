#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <stdbool.h>
#include <stdint.h>
#include <cstddef>
#include <string>
#include <chrono>
#include <vector>
#include <stdexcept>
#include <algorithm>


typedef int32_t contact_id;
typedef int32_t user_id;
typedef int32_t message_id;
typedef int32_t chat_id;

typedef enum {
    SENT,
    DELIVERED,
    READ,
    FAILED
} message_status;

typedef enum {
    MESSAGE_NORMAL = 0,   // Сообщение в один чат
    MESSAGE_BROADCAST = 1 // Сообщение во все чаты
} message_type;

typedef enum {
    PRIVATE,
    GROUP,
    CHANNEL
} chat_type;

constexpr size_t MAX_USERNAME_LENGTH = 100;
constexpr size_t MAX_CONTACT_NAME_LENGTH = 100;
constexpr size_t MAX_PASSWORD_LENGTH = 100;
constexpr size_t MAX_MESSAGE_LENGTH = 1000;

#endif
