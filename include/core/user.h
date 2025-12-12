#ifndef USER_H
#define USER_H

#include "types.h"
#include "contact.h"
#include <QString>
#include <QVector>
#include <QDateTime>
#include <mutex>

class User {
private:
    mutable std::mutex contacts_mutex;
    mutable std::mutex blocked_mutex;
    mutable std::mutex status_mutex;

public:
    user_id id;
    QString username;
    QString password;
    bool online;
    QDateTime lastSeen;
    QVector<Contact> contacts;
    QVector<Contact> blockedUsers;

    User(user_id id, QString username, QString password);
    ~User() = default;

    void addContact(const Contact& contact);
    void removeContact(contact_id contactId);
    QVector<Contact> getContacts() const;
    
    void blockUser(const Contact& user);
    void unblockUser(user_id userId);
    bool isBlocked(user_id userId) const;
    
    void setOnline(bool isOnline);
    bool getOnline() const;
    QDateTime getLastSeen() const;
};

#endif
