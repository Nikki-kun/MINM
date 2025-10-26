#include "core/сhat.h"

Chat::Chat(chat_id id, chat_type type, const std::vector<user_id>& participants, 
           const std::vector<message_id>& messages, std::chrono::system_clock::time_point created_date)
    : id(id), type(type), participants(participants), messages(messages), created_date(created_date) {
    
    if (participants.empty()) {
        throw std::invalid_argument("Чат должен содержать хотя бы одного участника");
    }
}

Chat::Chat(chat_id id, chat_type type, const std::vector<user_id>& participants)
    : Chat(id, type, participants, std::vector<message_id>(), std::chrono::system_clock::now()) {
}

chat_id Chat::getId() const {
    return id;
}

chat_type Chat::getType() const {
    return type;
}

const std::vector<user_id>& Chat::getParticipants() const {
    return participants;
}

void Chat::setParticipants(const std::vector<user_id>& participants) {
    if (participants.empty()) {
        throw std::invalid_argument("Чат должен содержать хотя бы одного участника");
    }
    this->participants = participants;
}

void Chat::addParticipant(user_id participant) {
    if (!hasParticipant(participant)) {
        participants.push_back(participant);
    }
}

void Chat::removeParticipant(user_id participant) {
    participants.erase(
        std::remove(participants.begin(), participants.end(), participant),
        participants.end()
    );
}

bool Chat::hasParticipant(user_id participant) const {
    return std::find(participants.begin(), participants.end(), participant) != participants.end();
}

size_t Chat::getParticipantsCount() const {
    return participants.size();
}

const std::vector<message_id>& Chat::getMessages() const {
    return messages;
}

void Chat::setMessages(const std::vector<message_id>& messages) {
    this->messages = messages;
}

void Chat::addMessage(message_id message) {
    if (!hasMessage(message)) {
        messages.push_back(message);
    }
}

void Chat::removeMessage(message_id message) {
    messages.erase(
        std::remove(messages.begin(), messages.end(), message),
        messages.end()
    );
}

bool Chat::hasMessage(message_id message) const {
    return std::find(messages.begin(), messages.end(), message) != messages.end();
}

size_t Chat::getMessagesCount() const {
    return messages.size();
}

std::chrono::system_clock::time_point Chat::getCreatedDate() const {
    return created_date;
}

bool Chat::isValid() const {
    return !participants.empty();
}