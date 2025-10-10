#pragma once
#include "types.h"
#include <string>

class Contact {
private:
	UserID owner_id;
	UserID contact_id;
	std::string alias;
	Timestamp added_date;

public:
	Contact(UserID owner, UserID contact, const std::string& contact_alias = "");

	UserID getOwnerId() const;
	UserID getContactId() const;
	std::string getAlias() const;
	Timestamp getAddedDate() const;

	void setAlias(const std::string& new_alias);
};