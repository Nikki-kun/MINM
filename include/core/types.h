#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <stdbool.h>
#include <stdint.h>

#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 100
#define MAX_CONTACT_NAME_LEN 50
#define MAX_CONTENT_LEN 1000

typedef int32_t contact_id;
typedef int32_t user_id;
typedef int32_t message_id;
typedef int32_t chat_id;

typedef enum {
    MSG_STATUS_SENT,
    MSG_STATUS_DELIVERED,
    MSG_STATUS_READ,
    MSG_STATUS_FAILED
} message_status;

typedef enum {
    CHAT_TYPE_PRIVATE,
    CHAT_TYPE_GROUP,
    CHAT_TYPE_CHANNEL
} chat_type;

#endif
