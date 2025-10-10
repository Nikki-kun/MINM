#pragma once
#include <vector>
#include <string>
#include <memory>
#include "types.h"
#include "message.h"

struct GroupParticipant {
	UserID user_id;
	ParticipantStatus status; 
	Timestamp join_date;
	UserID added_by; 

	GroupParticipant(UserID uid, UserID added_by_id);
};

class Group {
private:
	GroupID group_id;
	GroupType type;
	std::string title;
	std::string description;
	std::vector<GroupParticipant> participants;
	std::vector<Message> messages;
	Timestamp created_date;
	UserID owner_id;
	int max_participants;

public:
	Group(GroupID grp_id, GroupType grp_type, const std::string& grp_title, UserID own_id);

	bool addParticipant(UserID user_id, UserID added_by_user);
	bool removeParticipant(UserID user_id);
	bool updateParticipantStatus(UserID user_id, ParticipantStatus status);
	bool isUserInGroup(UserID user_id) const;

	void addMessage(const Message& message);
	std::vector<Message> getMessages() const;
	std::vector<Message> getMessagesSince(Timestamp since) const;

	std::vector<UserID> getParticipantIds() const;
	int getParticipantCount() const;
	bool isFull() const;

	GroupID getGroupId() const;
	GroupType getType() const;
	std::string getTitle() const;
	std::string getDescription() const;
	Timestamp getCreatedDate() const;
	UserID getOwnerId() const;
	int getMaxParticipants() const;

	void setTitle(const std::string& new_title);
	void setDescription(const std::string& new_description);
	void setMaxParticipants(int max);
};