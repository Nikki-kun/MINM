#pragma once
#include <string>
#include "types.h"

class User {
private:
	UserID user_id;
	std::string username;
	std::string display_name;
	Timestamp registration_date;
	bool is_online;

public:
	User(UserID id, const std::string& name, const std::string& display = "");

	// Getters
	UserID getUserId() const;
	std::string getUsername() const;
	std::string getDisplayName() const;
	Timestamp getRegistrationDate() const;
	bool getOnlineStatus() const;

	// Setters
	void setDisplayName(const std::string& name);
	void setOnlineStatus(bool online);
};