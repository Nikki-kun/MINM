#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include "../core/message.h"
#include "../core/group.h"

using NotificationCallback = std::function<void(const std::string& event_type, void* data)>;

class NotificationManager {
private:
    std::vector<NotificationCallback> callbacks;

public:
    void notifyNewMessage(const Message& message);
    void notifyMessageStatusChanged(MessageID msg_id, MessageStatus status);

    void notifyGroupCreated(const Group& group);
    void notifyParticipantAdded(GroupID group_id, UserID user_id);
    void notifyParticipantRemoved(GroupID group_id, UserID user_id);
    void notifyParticipantStatusChanged(GroupID group_id, UserID user_id, ParticipantStatus status);

    void notifyUserStatusChanged(UserID user_id, bool is_online);

    void subscribe(const NotificationCallback& callback);
    void unsubscribe(const NotificationCallback& callback);
};