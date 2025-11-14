#include "core/message.h"

template<typename ContentType>
Message<ContentType>::Message(message_id id, user_id sender_id, user_id receiver_id, 
                 ContentType content, std::chrono::system_clock::time_point timestamp)
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

template<typename ContentType>
Message<ContentType>::Message(message_id id, user_id sender_id, user_id receiver_id, ContentType content)
    : Message(id, sender_id, receiver_id, content, std::chrono::system_clock::now()) {
}

template<typename ContentType>
message_id Message<ContentType>::getId() const {
    return id;
}

template<typename ContentType>
user_id Message<ContentType>::getSenderId() const {
    return sender_id;
}

template<typename ContentType>
user_id Message<ContentType>::getReceiverId() const {
    return receiver_id;
}

template<typename ContentType>
ContentType Message<ContentType>::getContent() const {
    return content;
}

template<typename ContentType>
void Message<ContentType>::setContent(ContentType content) {
    if (!validateContent(content)) {
        throw std::invalid_argument("Сообщение не может быть пустым или превышать " + 
                                   std::to_string(MAX_MESSAGE_LENGTH) + " символов");
    }
    this->content = content;
}

template<typename ContentType>
std::chrono::system_clock::time_point Message<ContentType>::getTimestamp() const {
    return timestamp;
}

template<typename ContentType>
void Message<ContentType>::setTimestamp(std::chrono::system_clock::time_point timestamp) {
    this->timestamp = timestamp;
}

template<typename ContentType>
message_status Message<ContentType>::getStatus() const {
    return status;
}

template<typename ContentType>
void Message<ContentType>::setStatus(message_status status) {
    this->status = status;
}

template<typename ContentType>
bool Message<ContentType>::isValid() const {
    return validateContent(content) && sender_id != receiver_id;
}

template<typename ContentType>
bool Message<ContentType>::validateContent(const ContentType& content) {
    return !content.empty() && content.length() <= MAX_MESSAGE_LENGTH;
}