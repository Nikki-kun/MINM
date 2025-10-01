#ifndef MESSAGE_H
#define MESSAGE_H

#include "types.h"
#include <chrono>
#include <string>

class Message{
private:
    message_id id;
    user_id sender_id;
    user_id receiver_id;
    std::string content;
    std::chrono::system_clock::time_point timestamp;
    message_status status;

public:
    Message(message_id id, user_id sender_id, user_id receiver_id, std::string content, std::chrono::system_clock::time_point timestamp);
    ~Message();
    message_id getId();
    user_id getSenderId();
    void setSenderId(user_id sender_id);
    user_id getReceiverId();
    void setReceiverId(user_id receiver_id);
    std::string getContent();
    void setContent(std::string content);
    std::chrono::system_clock::time_point getTimestamp();
    void setTimestamp(std::chrono::system_clock::time_point timestamp);
    message_status getStatus();
    void setStatus(message_status status);
};

#endif

