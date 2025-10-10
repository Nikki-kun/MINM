#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "../core/user.h"

struct AuthSession {
	std::string token;
	UserID user_id;
	Timestamp created_at;
	Timestamp expires_at;
};

class AuthManager {
private:
	std::unordered_map<std::string, AuthSession> sessions;
	std::unordered_map<UserID, User> users;
	std::unordered_map<std::string, UserID> username_to_id;
	UserID next_user_id;

public:
	AuthManager();

	bool authenticate(const std::string& token, UserID& out_user_id);
	std::string createSession(UserID user_id);
	void invalidateSession(const std::string& token);

	User registerUser(const std::string& username, const std::string& display_name = "");
	User getUser(UserID user_id) const;
	User getUserByUsername(const std::string& username) const;
	bool userExists(const std::string& username) const;

	bool validateCredentials(const std::string& username, const std::string& password);
};