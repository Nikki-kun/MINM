#ifndef CHAT_H
#define CHAT_H

#include "types.h"
#include <vector>
#include <chrono>
#include <shared_mutex>

class Chat {
private:
    mutable std::shared_mutex participants_mutex;
    mutable std::shared_mutex messages_mutex;

public:
    chat_id id;
    chat_type type;
    std::vector<user_id> participants;
    std::vector<message_id> messages;
    std::chrono::system_clock::time_point created_date;

    Chat(chat_id id, chat_type type, const std::vector<user_id>& participants, 
         const std::vector<message_id>& messages, 
         std::chrono::system_clock::time_point created_date);
    Chat(chat_id id, chat_type type, const std::vector<user_id>& participants);
    ~Chat() = default;

    Chat& operator+(user_id participant);
    Chat& operator-(user_id participant);

    void addParticipant(user_id participant);
    void removeParticipant(user_id participant);
    bool hasParticipant(user_id participant) const;
    
    void addMessage(message_id msg_id);
    void removeMessage(message_id msg_id);
    
    std::vector<message_id> getMessages() const;
    std::vector<user_id> getParticipants() const;
};

#endif