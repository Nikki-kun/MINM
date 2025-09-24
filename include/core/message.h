#ifndef MESSAGE_H
#define MESSAGE_H

#include "types.h"

typedef struct {
    message_id message_id;
    user_id sender_id;
    user_id receiver_id;
    char content[MAX_CONTENT_LEN];
    time_t timestamp;
    message_status status;
} message;

#endif