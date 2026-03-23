#ifndef CHAT_H
#define CHAT_H

#include "types.h"
#include <vector>
#include <chrono>
#include <shared_mutex>
#include <unordered_map>
#include <optional>

struct ChatParticipantInfo {
    chat_participant_role role = CHAT_ROLE_MEMBER;
    chat_participant_status status = CHAT_MEMBER_ACTIVE;
    std::chrono::system_clock::time_point joined_at = std::chrono::system_clock::now();
    std::optional<std::chrono::system_clock::time_point> left_at;
    std::optional<std::chrono::system_clock::time_point> banned_at;
};

class Chat {
private:
    mutable std::shared_mutex participants_mutex;
    mutable std::shared_mutex messages_mutex;

public:
    chat_id id;
    chat_type type;
    std::vector<user_id> participants;
    std::vector<message_id> messages;
    std::chrono::system_clock::time_point created_date;
    std::unordered_map<user_id, ChatParticipantInfo> participantInfo;

    Chat(chat_id id, chat_type type, const std::vector<user_id>& participants, 
         const std::vector<message_id>& messages, 
         std::chrono::system_clock::time_point created_date);
    Chat(chat_id id, chat_type type, const std::vector<user_id>& participants);
    ~Chat() = default;

    Chat& operator+(user_id participant);
    Chat& operator-(user_id participant);

    void addParticipant(user_id participant);
    void addParticipant(user_id participant, const ChatParticipantInfo& info);
    void removeParticipant(user_id participant);
    bool hasParticipant(user_id participant) const;
    bool setParticipantInfo(user_id participant, const ChatParticipantInfo& info);
    std::optional<ChatParticipantInfo> getParticipantInfo(user_id participant) const;
    std::unordered_map<user_id, ChatParticipantInfo> getAllParticipantInfo() const;
    
    void addMessage(message_id msg_id);
    void removeMessage(message_id msg_id);
    
    std::vector<message_id> getMessages() const;
    std::vector<user_id> getParticipants() const;
};

#endif