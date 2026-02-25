#include "core/message.h"
#include <mutex>

template<typename ContentType>
Message<ContentType>::Message(message_id id, user_id sender_id, chat_id receiver_id, 
                              ContentType content, std::chrono::system_clock::time_point timestamp,
                              message_type type)
    : id(id), sender_id(sender_id), receiver_id(receiver_id), 
      content(content), timestamp(timestamp), status(SENT), type(type) {}

template<typename ContentType>
Message<ContentType>::Message(message_id id, user_id sender_id, chat_id receiver_id, 
                              ContentType content, message_type type)
    : id(id), sender_id(sender_id), receiver_id(receiver_id), 
      content(content), timestamp(std::chrono::system_clock::now()), status(SENT), type(type) {}

template class Message<std::string>;
template class Message<int>;
template class Message<double>;

template<typename ContentType>
void Message<ContentType>::setStatus(message_status newStatus) {
    std::lock_guard<std::mutex> lock(status_mutex);
    status = newStatus;
}

template<typename ContentType>
message_status Message<ContentType>::getStatus() const {
    std::lock_guard<std::mutex> lock(status_mutex);
    return status;
}

template<typename ContentType>
ContentType Message<ContentType>::getContent() const {
    std::lock_guard<std::mutex> lock(content_mutex);
    return content;
}

template<typename ContentType>
void Message<ContentType>::setContent(const ContentType& newContent) {
    std::lock_guard<std::mutex> lock(content_mutex);
    content = newContent;
}