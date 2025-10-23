#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include "../core/user.h"
#include <unordered_map>
#include <string>

class AuthManager {
private:
    std::unordered_map<std::string, User*> sessions;

public:
    AuthManager();
    ~AuthManager();
    
    User* login(const std::string& username, const std::string& password);
    void logout(user_id user_id);
    User* registerUser(const std::string& username, const std::string& password);
    
    bool isUserLoggedIn(user_id user_id) const;
    void clearExpiredSessions();
};

#endif