#ifndef USER_H
#define USER_H

#include "types.h"
#include "contact.h"
#include <chrono>
#include <string>
#include <vector>

class User {
    private:
        user_id id;
        std::string username;
        std::string password;
        bool online;
        std::chrono::system_clock lastSeen;
        std::vector<Contact>* contacts;
        std::vector<Contact>* blockedUsers;
    public:
        User(user_id id, std::string username, std::string password);
        ~User();
        user_id getId();
        std::string getUserName();
        void setUserName(std::string name);
        bool isOnline();
        void setOnline(bool status);
        std::chrono::system_clock getLastSeen();
        void setLastSeen(std::chrono::system_clock time);
        std::vector<Contact> getContacts();
        void setContacts(std::vector<Contact> contacts);
        void getBlockedUsers();
        std::vector<Contact> setBlockedUsers();
};

#endif
