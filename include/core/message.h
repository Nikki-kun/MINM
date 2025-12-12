#ifndef MESSAGE_H
#define MESSAGE_H

#include "types.h"
#include <chrono>
#include <string>
#include <mutex>

template<typename ContentType = std::string>
class Message {
private:
    mutable std::mutex status_mutex;
    mutable std::mutex content_mutex;

public:
    message_id id;
    user_id sender_id;
    user_id receiver_id;
    ContentType content;
    std::chrono::system_clock::time_point timestamp;
    message_status status;

    Message(message_id id, user_id sender_id, user_id receiver_id, 
            ContentType content, std::chrono::system_clock::time_point timestamp);
    Message(message_id id, user_id sender_id, user_id receiver_id, ContentType content);
    ~Message() = default;

    void setStatus(message_status newStatus);
    message_status getStatus() const;
    
    ContentType getContent() const;
    void setContent(const ContentType& newContent);
};

#endif