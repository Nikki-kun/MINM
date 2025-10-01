#ifndef MESSAGE_H
#define MESSAGE_H

#include "types.h"
#include <chrono>
#include <string>

class Message{
    message_id id;
    user_id sender_id;
    user_id receiver_id;
    std::string content;
    std::chrono::system_clock timestamp;
    message_status status;
};

#endif
