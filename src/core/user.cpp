#include "core/user.h"
#include <QDebug>

User::User(user_id id, QString username, QString password) 
    : id(id), username(username), password(password), online(false),
      lastSeen(QDateTime::currentDateTime()) {
    
    if (!validateUsername(username)) {
        qWarning() << "Неверное имя пользователя:" << username;
        this->username = "InvalidUser";
    }
    
    if (!validatePassword(password)) {
        qWarning() << "Неверный пароль для пользователя:" << username;
        this->password = "default123";
    }
}

user_id User::getId() const {
    return id;
}

QString User::getUserName() const {
    return username;
}

bool User::setUserName(QString name) {
    if (!validateUsername(name)) {
        qWarning() << "Попытка установить недопустимое имя пользователя:" << name;
        return false;
    }
    username = name;
    return true;
}

QString User::getPassword() const {
    return password;
}

bool User::setPassword(QString password) {
    if (!validatePassword(password)) {
        qWarning() << "Попытка установить недопустимый пароль";
        return false;
    }
    this->password = password;
    return true;
}

bool User::isOnline() const {
    return online;
}

void User::setOnline(bool status) {
    online = status;
    if (!status) {
        lastSeen = QDateTime::currentDateTime();
    }
}

QDateTime User::getLastSeen() const {
    return lastSeen;
}

void User::setLastSeen(QDateTime time) {
    lastSeen = time;
}

const QVector<Contact>& User::getContacts() const {
    return contacts;
}

void User::setContacts(const QVector<Contact>& contacts) {
    this->contacts = contacts;
}

bool User::addContact(const Contact& contact) {
    if (!hasContact(contact.getId())) {
        contacts.push_back(contact);
        return true;
    }
    qDebug() << "Контакт уже существует:" << contact.getId();
    return false;
}

bool User::removeContact(user_id contactId) {
    int initialSize = contacts.size();
    contacts.erase(
        std::remove_if(contacts.begin(), contacts.end(),
            [contactId](const Contact& contact) {
                return contact.getId() == contactId;
            }),
        contacts.end()
    );
    bool removed = (contacts.size() < initialSize);
    if (!removed) {
        qDebug() << "Контакт не найден для удаления:" << contactId;
    }
    return removed;
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

int User::getContactsCount() const {
    return contacts.size();
}

void User::clearContacts() {
    contacts.clear();
}

const QVector<Contact>& User::getBlockedUsers() const {
    return blockedUsers;
}

void User::setBlockedUsers(const QVector<Contact>& blockedUsers) {
    this->blockedUsers = blockedUsers;
}

bool User::blockUser(const Contact& user) {
    if (!isUserBlocked(user.getId())) {
        blockedUsers.push_back(user);
        return true;
    }
    qDebug() << "Пользователь уже заблокирован:" << user.getId();
    return false;
}

bool User::unblockUser(user_id userId) {
    int initialSize = blockedUsers.size();
    blockedUsers.erase(
        std::remove_if(blockedUsers.begin(), blockedUsers.end(),
            [userId](const Contact& user) {
                return user.getId() == userId;
            }),
        blockedUsers.end()
    );
    bool unblocked = (blockedUsers.size() < initialSize);
    if (!unblocked) {
        qDebug() << "Пользователь не найден в блокировках:" << userId;
    }
    return unblocked;
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

int User::getBlockedUsersCount() const {
    return blockedUsers.size();
}

void User::clearBlockedUsers() {
    blockedUsers.clear();
}

bool User::validateUsername(const QString& username) {
    if (username.isEmpty() || username.length() > MAX_USERNAME_LENGTH) {
        return false;
    }
    
    return !username.contains(' ');
}

bool User::validatePassword(const QString& password) {
    return password.length() >= 4 && password.length() <= MAX_PASSWORD_LENGTH;
}

QString User::getLastSeenString() const {
    return lastSeen.toString("dd.MM.yyyy HH:mm");
}

bool User::isValid() const {
    return validateUsername(username) && validatePassword(password);
}