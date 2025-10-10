#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include "../core/message.h"
#include "../core/types.h"

class MessageManager {
private:
	std::unordered_map<MessageID, Message> messages;
	std::unordered_map<GroupID, std::vector<MessageID>> group_messages;
	std::unordered_map<MessageID, std::vector<UserID>> message_read_by;
	MessageID next_message_id;

public:
	MessageManager();

	Message sendMessage(GroupID group_id, UserID author_id, const std::string& content);
	Message getMessage(MessageID message_id) const;
	std::vector<Message> getGroupMessages(GroupID group_id) const;
	std::vector<Message> getGroupMessagesSince(GroupID group_id, Timestamp since) const;

	void markMessageAsDelivered(MessageID message_id);
	void markMessageAsRead(MessageID message_id, UserID user_id);
	MessageStatus getMessageStatus(MessageID message_id) const;
	bool isMessageReadByUser(MessageID message_id, UserID user_id) const;

	std::vector<Message> searchInGroup(GroupID group_id, const std::string& query) const;
};