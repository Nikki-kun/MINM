#include "core/message.h"

Message::Message(message_id id, user_id sender_id, user_id receiver_id, 
                 std::string content, std::chrono::system_clock::time_point timestamp)
    : id(id), sender_id(sender_id), receiver_id(receiver_id), 
      content(content), timestamp(timestamp), status(message_status::SENT) {
    
    if (!validateContent(content)) {
        throw std::invalid_argument("Сообщение не может быть пустым или превышать " + 
                                   std::to_string(MAX_MESSAGE_LENGTH) + " символов");
    }
    if (sender_id == receiver_id) {
        throw std::invalid_argument("Отправитель и получатель не могут быть одним пользователем");
    }
}

Message::Message(message_id id, user_id sender_id, user_id receiver_id, std::string content)
    : Message(id, sender_id, receiver_id, content, std::chrono::system_clock::now()) {
}

message_id Message::getId() const {
    return id;
}

user_id Message::getSenderId() const {
    return sender_id;
}

user_id Message::getReceiverId() const {
    return receiver_id;
}

std::string Message::getContent() const {
    return content;
}

void Message::setContent(std::string content) {
    if (!validateContent(content)) {
        throw std::invalid_argument("Сообщение не может быть пустым или превышать " + 
                                   std::to_string(MAX_MESSAGE_LENGTH) + " символов");
    }
    this->content = content;
}

std::chrono::system_clock::time_point Message::getTimestamp() const {
    return timestamp;
}

void Message::setTimestamp(std::chrono::system_clock::time_point timestamp) {
    this->timestamp = timestamp;
}

message_status Message::getStatus() const {
    return status;
}

void Message::setStatus(message_status status) {
    this->status = status;
}

bool Message::isValid() const {
    return validateContent(content) && sender_id != receiver_id;
}

bool Message::validateContent(const std::string& content) {
    return !content.empty() && content.length() <= MAX_MESSAGE_LENGTH;
}