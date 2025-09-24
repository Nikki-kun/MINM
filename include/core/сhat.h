#ifndef CHAT_H
#define CHAT_H

#include "types.h"
#include "message.h"

typedef struct {
    chat_id chat_id;
    chat_type type;
    list* participants;
    list* messages;
    time_t created_date;
} chat;

#endif