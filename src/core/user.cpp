#include "core/user.h"
#include <algorithm>
#include <stdexcept>

User::User(user_id id, std::string username, std::string password) 
    : id(id), username(username), password(password), online(false),
      lastSeen(std::chrono::system_clock::now()) {
    
    if (!validateUsername(username)) {
        throw std::invalid_argument("Имя пользователя не может быть пустым, содержать пробелы или превышать " + std::to_string(MAX_USERNAME_LENGTH) + " символов");
    }
    
    if (!validatePassword(password)) {
        throw std::invalid_argument("Пароль должен содержать от 4 до " + std::to_string(MAX_PASSWORD_LENGTH) + " символов");
    }
}

user_id User::getId() const {
    return id;
}

std::string User::getUserName() const {
    return username;
}

void User::setUserName(std::string name) {
    if (!validateUsername(name)) {
        throw std::invalid_argument("Имя пользователя не может быть пустым, содержать пробелы или превышать " + std::to_string(MAX_USERNAME_LENGTH) + " символов");
    }
    username = name;
}

std::string User::getPassword() const {
    return password;
}

void User::setPassword(std::string password) {
    if (!validatePassword(password)) {
        throw std::invalid_argument("Пароль должен содержать от 4 до " + std::to_string(MAX_PASSWORD_LENGTH) + " символов");
    }
    this->password = password;
}

bool User::isOnline() const {
    return online;
}

void User::setOnline(bool status) {
    online = status;
    if (!status) {
        lastSeen = std::chrono::system_clock::now();
    }
}

std::chrono::system_clock::time_point User::getLastSeen() const {
    return lastSeen;
}

void User::setLastSeen(std::chrono::system_clock::time_point time) {
    lastSeen = time;
}

const std::vector<Contact>& User::getContacts() const {
    return contacts;
}

void User::setContacts(const std::vector<Contact>& contacts) {
    this->contacts = contacts;
}

void User::addContact(const Contact& contact) {
    if (!hasContact(contact.getId())) {
        contacts.push_back(contact);
    }
}

void User::removeContact(user_id contactId) {
    contacts.erase(
        std::remove_if(contacts.begin(), contacts.end(),
            [contactId](const Contact& contact) {
                return contact.getId() == contactId;
            }),
        contacts.end()
    );
}

bool User::hasContact(user_id contactId) const {
    return std::any_of(contacts.begin(), contacts.end(),
        [contactId](const Contact& contact) {
            return contact.getId() == contactId;
        });
}

Contact* User::findContact(user_id contactId) {
    auto it = std::find_if(contacts.begin(), contacts.end(),
        [contactId](const Contact& contact) {
            return contact.getId() == contactId;
        });
    return it != contacts.end() ? &(*it) : nullptr;
}

const Contact* User::findContact(user_id contactId) const {
    auto it = std::find_if(contacts.begin(), contacts.end(),
        [contactId](const Contact& contact) {
            return contact.getId() == contactId;
        });
    return it != contacts.end() ? &(*it) : nullptr;
}

size_t User::getContactsCount() const {
    return contacts.size();
}

void User::clearContacts() {
    contacts.clear();
}

const std::vector<Contact>& User::getBlockedUsers() const {
    return blockedUsers;
}

void User::setBlockedUsers(const std::vector<Contact>& blockedUsers) {
    this->blockedUsers = blockedUsers;
}

void User::blockUser(const Contact& user) {
    if (!isUserBlocked(user.getId())) {
        blockedUsers.push_back(user);
    }
}

void User::unblockUser(user_id userId) {
    blockedUsers.erase(
        std::remove_if(blockedUsers.begin(), blockedUsers.end(),
            [userId](const Contact& user) {
                return user.getId() == userId;
            }),
        blockedUsers.end()
    );
}

bool User::isUserBlocked(user_id userId) const {
    return std::any_of(blockedUsers.begin(), blockedUsers.end(),
        [userId](const Contact& user) {
            return user.getId() == userId;
        });
}

Contact* User::findBlockedUser(user_id userId) {
    auto it = std::find_if(blockedUsers.begin(), blockedUsers.end(),
        [userId](const Contact& user) {
            return user.getId() == userId;
        });
    return it != blockedUsers.end() ? &(*it) : nullptr;
}

const Contact* User::findBlockedUser(user_id userId) const {
    auto it = std::find_if(blockedUsers.begin(), blockedUsers.end(),
        [userId](const Contact& user) {
            return user.getId() == userId;
        });
    return it != blockedUsers.end() ? &(*it) : nullptr;
}

size_t User::getBlockedUsersCount() const {
    return blockedUsers.size();
}

void User::clearBlockedUsers() {
    blockedUsers.clear();
}

bool User::validateUsername(const std::string& username) {
    if (username.empty() || username.length() > MAX_USERNAME_LENGTH) {
        return false;
    }
    
    return username.find(' ') == std::string::npos;
}

bool User::validatePassword(const std::string& password) {
    return password.length() >= 4 && password.length() <= MAX_PASSWORD_LENGTH;
}