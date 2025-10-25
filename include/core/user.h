#ifndef USER_H
#define USER_H

#include "types.h"
#include "contact.h"
#include <chrono>
#include <string>
#include <vector>
#include <stdexcept>

class User {
private:
    user_id id;
    std::string username;
    std::string password;
    bool online;
    std::chrono::system_clock::time_point lastSeen;
    std::vector<Contact> contacts;
    std::vector<Contact> blockedUsers;

public:
    User(user_id id, std::string username, std::string password);
    ~User() = default;

    user_id getId() const;
    std::string getUserName() const;
    void setUserName(std::string name);
    std::string getPassword() const;
    void setPassword(std::string password);
    bool isOnline() const;
    void setOnline(bool status);
    std::chrono::system_clock::time_point getLastSeen() const;
    void setLastSeen(std::chrono::system_clock::time_point time);

    const std::vector<Contact>& getContacts() const;
    void setContacts(const std::vector<Contact>& contacts);
    void addContact(const Contact& contact);
    void removeContact(user_id contactId);
    bool hasContact(user_id contactId) const;
    Contact* findContact(user_id contactId);
    const Contact* findContact(user_id contactId) const;
    size_t getContactsCount() const;
    void clearContacts();

    const std::vector<Contact>& getBlockedUsers() const;
    void setBlockedUsers(const std::vector<Contact>& blockedUsers);
    void blockUser(const Contact& user);
    void unblockUser(user_id userId);
    bool isUserBlocked(user_id userId) const;
    Contact* findBlockedUser(user_id userId);
    const Contact* findBlockedUser(user_id userId) const;
    size_t getBlockedUsersCount() const;
    void clearBlockedUsers();

    static bool validateUsername(const std::string& username);
    static bool validatePassword(const std::string& password);
};

#endif