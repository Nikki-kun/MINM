#include "chat.h"
#include <algorithm>
#include <mutex>
#include <shared_mutex>

Chat::Chat(chat_id id, chat_type type, const std::vector<user_id>& participants, 
           const std::vector<message_id>& messages, 
           std::chrono::system_clock::time_point created_date)
    : id(id), type(type), participants(participants), 
      messages(messages), created_date(created_date) {}

Chat::Chat(chat_id id, chat_type type, const std::vector<user_id>& participants)
    : id(id), type(type), participants(participants), 
      created_date(std::chrono::system_clock::now()) {}

Chat& Chat::operator+(user_id participant) {
    std::unique_lock<std::shared_mutex> lock(participants_mutex);
    
    if (std::find(participants.begin(), participants.end(), participant) == participants.end()) {
        participants.push_back(participant);
    }
    
    return *this;
}

Chat& Chat::operator-(user_id participant) {
    std::unique_lock<std::shared_mutex> lock(participants_mutex);
    
    auto it = std::find(participants.begin(), participants.end(), participant);
    if (it != participants.end()) {
        participants.erase(it);
    }
    
    return *this;
}

void Chat::addParticipant(user_id participant) {
    std::unique_lock<std::shared_mutex> lock(participants_mutex);
    
    if (std::find(participants.begin(), participants.end(), participant) == participants.end()) {
        participants.push_back(participant);
    }
}

void Chat::removeParticipant(user_id participant) {
    std::unique_lock<std::shared_mutex> lock(participants_mutex);
    
    auto it = std::find(participants.begin(), participants.end(), participant);
    if (it != participants.end()) {
        participants.erase(it);
    }
}

bool Chat::hasParticipant(user_id participant) const {
    std::shared_lock<std::shared_mutex> lock(participants_mutex);
    return std::find(participants.begin(), participants.end(), participant) != participants.end();
}

void Chat::addMessage(message_id msg_id) {
    std::unique_lock<std::shared_mutex> lock(messages_mutex);
    messages.push_back(msg_id);
}

void Chat::removeMessage(message_id msg_id) {
    std::unique_lock<std::shared_mutex> lock(messages_mutex);
    
    auto it = std::find(messages.begin(), messages.end(), msg_id);
    if (it != messages.end()) {
        messages.erase(it);
    }
}

std::vector<message_id> Chat::getMessages() const {
    std::shared_lock<std::shared_mutex> lock(messages_mutex);
    return messages;
}

std::vector<user_id> Chat::getParticipants() const {
    std::shared_lock<std::shared_mutex> lock(participants_mutex);
    return participants;
}