#ifndef USER_H
#define USER_H

#include "types.h"
#include "contact.h"
#include <QString>
#include <QVector>
#include <QDateTime>

class User {
private:
    user_id id;
    QString username;
    QString password;
    bool online;
    QDateTime lastSeen;
    QVector<Contact> contacts;
    QVector<Contact> blockedUsers;

public:
    User(user_id id, QString username, QString password);
    ~User() = default;

    user_id getId() const;
    QString getUserName() const;
    bool setUserName(QString name);
    QString getPassword() const;
    bool setPassword(QString password);
    bool isOnline() const;
    void setOnline(bool status);
    QDateTime getLastSeen() const;
    QString getLastSeenString() const;
    void setLastSeen(QDateTime time);

    const QVector<Contact>& getContacts() const;
    void setContacts(const QVector<Contact>& contacts);
    bool addContact(const Contact& contact);
    bool removeContact(user_id contactId);
    bool hasContact(user_id contactId) const;
    Contact* findContact(user_id contactId);
    const Contact* findContact(user_id contactId) const;
    int getContactsCount() const;
    void clearContacts();

    const QVector<Contact>& getBlockedUsers() const;
    void setBlockedUsers(const QVector<Contact>& blockedUsers);
    bool blockUser(const Contact& user);
    bool unblockUser(user_id userId);
    bool isUserBlocked(user_id userId) const;
    Contact* findBlockedUser(user_id userId);
    const Contact* findBlockedUser(user_id userId) const;
    int getBlockedUsersCount() const;
    void clearBlockedUsers();

    bool isValid() const;
    static bool validateUsername(const QString& username);
    static bool validatePassword(const QString& password);
};

#endif