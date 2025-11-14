#ifndef MESSAGE_H
#define MESSAGE_H

#include "types.h"

template<typename ContentType = std::string>
class Message {
private:
    message_id id;
    user_id sender_id;
    user_id receiver_id;
    ContentType content;
    std::chrono::system_clock::time_point timestamp;
    message_status status;

public:
    Message(message_id id, user_id sender_id, user_id receiver_id, ContentType content, std::chrono::system_clock::time_point timestamp);
    Message(message_id id, user_id sender_id, user_id receiver_id, ContentType content);
    ~Message() = default;

    message_id getId() const;
    user_id getSenderId() const;
    user_id getReceiverId() const;
    ContentType getContent() const;
    void setContent(ContentType content);
    std::chrono::system_clock::time_point getTimestamp() const;
    void setTimestamp(std::chrono::system_clock::time_point timestamp);
    message_status getStatus() const;
    void setStatus(message_status status);
    
    bool isValid() const;
    static bool validateContent(const ContentType& content);
};

#endif