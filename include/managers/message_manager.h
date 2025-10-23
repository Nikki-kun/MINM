#ifndef MESSAGE_MANAGER_H
#define MESSAGE_MANAGER_H

#include "../core/message.h"
#include "../core/user.h"
#include <unordered_map>
#include <vector>

class MessageManager {
private:
    std::unordered_map<message_id, Message*> messages;
    std::unordered_map<user_id, std::vector<Message*>*> undelivered_messages;

public:
    MessageManager();
    ~MessageManager();
    
    Message* sendMessage(user_id sender_id, user_id receiver_id, const std::string& content);
    std::vector<Message*> getMessages(user_id id, user_id contact_id) const;
    void updateMessageStatus(message_id id, message_status status);
    
    std::vector<Message*> getUndeliveredMessages(user_id id) const;
    void markMessagesAsDelivered(user_id id);
    
    MessageManager& operator+(Message* message);
    MessageManager& operator-(Message* message);
    MessageManager& operator+(const std::vector<Message*>& messages_list);
    MessageManager& operator-(const std::vector<Message*>& messages_list);
};

#endif