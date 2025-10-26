#ifndef CHAT_H
#define CHAT_H

#include "types.h"

class Chat {
private:
    chat_id id;
    chat_type type;
    std::vector<user_id> participants;
    std::vector<message_id> messages;
    std::chrono::system_clock::time_point created_date;

public:
    Chat(chat_id id, chat_type type, const std::vector<user_id>& participants, const std::vector<message_id>& messages, std::chrono::system_clock::time_point created_date);
    Chat(chat_id id, chat_type type, const std::vector<user_id>& participants);
    ~Chat() = default;

    chat_id getId() const;
    chat_type getType() const;
    
    const std::vector<user_id>& getParticipants() const;
    void setParticipants(const std::vector<user_id>& participants);
    void addParticipant(user_id participant);
    void removeParticipant(user_id participant);
    bool hasParticipant(user_id participant) const;
    size_t getParticipantsCount() const;
    
    const std::vector<message_id>& getMessages() const;
    void setMessages(const std::vector<message_id>& messages);
    void addMessage(message_id message);
    void removeMessage(message_id message);
    bool hasMessage(message_id message) const;
    size_t getMessagesCount() const;
    
    std::chrono::system_clock::time_point getCreatedDate() const;
    
    bool isValid() const;
};

#endif