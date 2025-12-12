#include "user.h"
#include <mutex>

User::User(user_id id, QString username, QString password)
    : id(id), username(username), password(password), 
      online(false), lastSeen(QDateTime::currentDateTime()) {}

void User::addContact(const Contact& contact) {
    std::lock_guard<std::mutex> lock(contacts_mutex);
    contacts.append(contact);
}

void User::removeContact(contact_id contactId) {
    std::lock_guard<std::mutex> lock(contacts_mutex);
    
    for (int i = 0; i < contacts.size(); ++i) {
        if (contacts[i].id == contactId) {
            contacts.remove(i);
            break;
        }
    }
}

QVector<Contact> User::getContacts() const {
    std::lock_guard<std::mutex> lock(contacts_mutex);
    return contacts;
}

void User::blockUser(const Contact& user) {
    std::lock_guard<std::mutex> lock(blocked_mutex);
    blockedUsers.append(user);
}

void User::unblockUser(user_id userId) {
    std::lock_guard<std::mutex> lock(blocked_mutex);
    
    for (int i = 0; i < blockedUsers.size(); ++i) {
        if (blockedUsers[i].contactId == userId) {
            blockedUsers.remove(i);
            break;
        }
    }
}

bool User::isBlocked(user_id userId) const {
    std::lock_guard<std::mutex> lock(blocked_mutex);
    
    for (const auto& blocked : blockedUsers) {
        if (blocked.contactId == userId) {
            return true;
        }
    }
    return false;
}

void User::setOnline(bool isOnline) {
    std::lock_guard<std::mutex> lock(status_mutex);
    online = isOnline;
    lastSeen = QDateTime::currentDateTime();
}

bool User::getOnline() const {
    std::lock_guard<std::mutex> lock(status_mutex);
    return online;
}

QDateTime User::getLastSeen() const {
    std::lock_guard<std::mutex> lock(status_mutex);
    return lastSeen;
}