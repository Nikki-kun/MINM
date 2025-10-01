#ifndef CHAT_H
#define CHAT_H

#include "types.h"
#include <chrono>
#include <vector>

class Chat {
private:
    chat_id id;
    chat_type type;
    std::vector<user_id>* participants;
    std::vector<message_id>* messages;
    std::chrono::system_clock::time_point created_date;

public:
    Chat(chat_id id, chat_type type, std::vector<user_id>* participants, std::vector<message_id>* messages, std::chrono::system_clock::time_point created_date);
    ~Chat();
    chat_id getId();
    chat_type getType();
    void setType(chat_type type);
    std::vector<user_id>* getParticipants();
    void setParticipants(std::vector<user_id>* participants);
    std::vector<message_id>* getMessages();
    void setMessages(std::vector<message_id>* messages);
    std::chrono::system_clock::time_point getCreatedDate();
    void setCreatedDate(std::chrono::system_clock::time_point created_date);
};

#endif
