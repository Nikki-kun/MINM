#ifndef CHAT_H
#define CHAT_H

#include "types.h"
#include <chrono>
#include <vector>

class Chat {
    chat_id id;
    chat_type type;
    std::vector<user_id>* participants;
    std::vector<message_id>* messages;
    std::chrono::system_clock created_date;
};

#endif
