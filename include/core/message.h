#pragma once
#include <string>
#include <vector>
#include "types.h"

class Message {
private:
	MessageID message_id;
	GroupID group_id;
	UserID author_id;
	std::string content;
	Timestamp timestamp;
	MessageStatus status;
	std::vector<std::string> attachments;

public:
	Message(MessageID msg_id, GroupID grp_id, UserID auth_id, const std::string& content);

	// Getters
	MessageID getMessageId() const;
	GroupID getGroupId() const;
	UserID getAuthorId() const;
	std::string getContent() const;
	Timestamp getTimestamp() const;
	MessageStatus getStatus() const;
	std::vector<std::string> getAttachments() const;

	// Setters
	void setStatus(MessageStatus new_status);
	void addAttachment(const std::string& attachment_path);
	void setContent(const std::string& new_content);
};
