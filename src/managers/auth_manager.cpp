#include "managers/auth_manager.h"
#include <QNetworkRequest>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

AuthManager::AuthManager(const QString& serverUrl) 
    : serverBaseUrl(serverUrl), settings("MINM", "AuthManager") {
    networkManager = new QNetworkAccessManager(this);
    
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AuthManager::handleLoginReply);
    
    loadUsers();
}

AuthManager::~AuthManager() {
    for (auto& session : sessions) {
        delete session;
    }
    sessions.clear();
}

void AuthManager::login(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) {
        qWarning() << "Попытка входа с пустыми данными";
        emit loginFailed("Имя пользователя и пароль не могут быть пустыми");
        return;
    }
    
    QUrl url(serverBaseUrl + "/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    
    disconnect(networkManager, &QNetworkAccessManager::finished,
               this, &AuthManager::handleLoginReply);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AuthManager::handleLoginReply);
    
    networkManager->post(request, data);
}

void AuthManager::logout(user_id user_id) {
    QString sessionToken;
    for (auto it = sessions.begin(); it != sessions.end(); ++it) {
        if (it.value()->getId() == user_id) {
            sessionToken = it.key();
            break;
        }
    }
    
    if (sessionToken.isEmpty()) {
        qWarning() << "Попытка выхода неавторизованного пользователя:" << user_id;
        emit logoutFailed(user_id, "Пользователь не авторизован");
        return;
    }
    
    QUrl url(serverBaseUrl + "/auth/logout");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + sessionToken).toUtf8());
    
    disconnect(networkManager, &QNetworkAccessManager::finished,
               this, &AuthManager::handleLogoutReply);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AuthManager::handleLogoutReply);
    
    QNetworkReply* reply = networkManager->post(request, QByteArray());
    reply->setProperty("user_id", QVariant::fromValue(user_id));
}

void AuthManager::registerUser(const QString& username, const QString& password) {
    if (!User::validateUsername(username)) {
        qWarning() << "Попытка регистрации с невалидным именем пользователя:" << username;
        emit registrationFailed("Недопустимое имя пользователя");
        return;
    }
    
    if (!User::validatePassword(password)) {
        qWarning() << "Попытка регистрации с невалидным паролем";
        emit registrationFailed("Недопустимый пароль");
        return;
    }
    
    QUrl url(serverBaseUrl + "/auth/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    
    disconnect(networkManager, &QNetworkAccessManager::finished,
               this, &AuthManager::handleRegistrationReply);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AuthManager::handleRegistrationReply);
    
    networkManager->post(request, data);
}

void AuthManager::handleLoginReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "Невалидный JSON ответ при входе";
            emit loginFailed("Ошибка сервера: невалидный ответ");
            reply->deleteLater();
            return;
        }
        
        QJsonObject json = doc.object();
        
        if (json["success"].toBool()) {
            User* user = parseUserFromJson(json["user"].toObject());
            QString token = json["token"].toString();
            
            if (user && !token.isEmpty()) {
                sessions[token] = user;
                saveUsers();
                emit loginSuccess(user);
            } else {
                qWarning() << "Ошибка парсинга пользователя или токена";
                emit loginFailed("Ошибка обработки данных пользователя");
                delete user;
            }
        } else {
            QString error = json["error"].toString("Неизвестная ошибка");
            qWarning() << "Ошибка входа:" << error;
            emit loginFailed(error);
        }
    } else {
        QString error = QString("Ошибка сети: %1").arg(reply->errorString());
        qWarning() << "Сетевая ошибка при входе:" << error;
        emit loginFailed(error);
    }
    
    reply->deleteLater();
}

void AuthManager::handleRegistrationReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "Невалидный JSON ответ при регистрации";
            emit registrationFailed("Ошибка сервера: невалидный ответ");
            reply->deleteLater();
            return;
        }
        
        QJsonObject json = doc.object();
        
        if (json["success"].toBool()) {
            User* user = parseUserFromJson(json["user"].toObject());
            QString token = json["token"].toString();
            
            if (user && !token.isEmpty()) {
                sessions[token] = user;
                saveUsers();
                emit registrationSuccess(user);
            } else {
                qWarning() << "Ошибка парсинга пользователя или токена при регистрации";
                emit registrationFailed("Ошибка обработки данных пользователя");
                delete user;
            }
        } else {
            QString error = json["error"].toString("Неизвестная ошибка");
            qWarning() << "Ошибка регистрации:" << error;
            emit registrationFailed(error);
        }
    } else {
        QString error = QString("Ошибка сети: %1").arg(reply->errorString());
        qWarning() << "Сетевая ошибка при регистрации:" << error;
        emit registrationFailed(error);
    }
    
    reply->deleteLater();
}

void AuthManager::handleLogoutReply(QNetworkReply* reply) {
    user_id user_id = reply->property("user_id").toLongLong();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject json = doc.object();
            
            if (json["success"].toBool()) {
                QString sessionToken;
                for (auto it = sessions.begin(); it != sessions.end(); ++it) {
                    if (it.value()->getId() == user_id) {
                        sessionToken = it.key();
                        delete it.value();
                        sessions.remove(sessionToken);
                        saveUsers();
                        break;
                    }
                }
                emit logoutSuccess(user_id);
            } else {
                QString error = json["error"].toString("Неизвестная ошибка");
                qWarning() << "Ошибка выхода:" << error;
                emit logoutFailed(user_id, error);
            }
        } else {
            qWarning() << "Невалидный JSON ответ при выходе";
            emit logoutFailed(user_id, "Ошибка сервера: невалидный ответ");
        }
    } else {
        QString error = QString("Ошибка сети: %1").arg(reply->errorString());
        qWarning() << "Сетевая ошибка при выходе:" << error;
        emit logoutFailed(user_id, error);
    }
    
    reply->deleteLater();
}

User* AuthManager::parseUserFromJson(const QJsonObject& json) {
    if (json.isEmpty()) {
        qWarning() << "Пустой JSON объект для парсинга пользователя";
        return nullptr;
    }
    
    user_id id = json["id"].toVariant().toLongLong();
    QString username = json["username"].toString();
    QString password = json["password"].toString();
    
    if (id == 0 || username.isEmpty()) {
        qWarning() << "Невалидные данные пользователя в JSON";
        return nullptr;
    }
    
    User* user = new User(id, username, password);
    user->setOnline(json["online"].toBool());
    
    qint64 lastSeenMs = json["lastSeen"].toVariant().toLongLong();
    QDateTime lastSeen = QDateTime::fromMSecsSinceEpoch(lastSeenMs);
    user->setLastSeen(lastSeen);
    
    QJsonArray contactsArray = json["contacts"].toArray();
    QVector<Contact> contacts;
    for (const QJsonValue& contactValue : contactsArray) {
        Contact contact = parseContactFromJson(contactValue.toObject());
        if (contact.getId() != 0) {
            contacts.push_back(contact);
        }
    }
    user->setContacts(contacts);
    
    QJsonArray blockedArray = json["blockedUsers"].toArray();
    QVector<Contact> blockedUsers;
    for (const QJsonValue& blockedValue : blockedArray) {
        Contact contact = parseContactFromJson(blockedValue.toObject());
        if (contact.getId() != 0) {
            blockedUsers.push_back(contact);
        }
    }
    user->setBlockedUsers(blockedUsers);
    
    return user;
}

Contact AuthManager::parseContactFromJson(const QJsonObject& json) {
    if (json.isEmpty()) {
        return Contact(0, 0, 0, "", QDateTime::currentDateTime());
    }
    
    contact_id id = json["id"].toVariant().toLongLong();
    user_id ownerId = json["ownerId"].toVariant().toLongLong();
    user_id contactId = json["contactId"].toVariant().toLongLong();
    QString contactName = json["contactName"].toString();
    
    qint64 addedDateMs = json["addedDate"].toVariant().toLongLong();
    QDateTime addedDate = QDateTime::fromMSecsSinceEpoch(addedDateMs);
    
    if (id == 0 || ownerId == 0 || contactId == 0) {
        qWarning() << "Невалидные данные контакта в JSON";
        return Contact(0, 0, 0, "", QDateTime::currentDateTime());
    }
    
    return Contact(id, ownerId, contactId, contactName, addedDate);
}

bool AuthManager::isUserLoggedIn(user_id user_id) const {
    for (auto it = sessions.begin(); it != sessions.end(); ++it) {
        if (it.value()->getId() == user_id) {
            return true;
        }
    }
    return false;
}

void AuthManager::clearExpiredSessions() {
    QDateTime currentTime = QDateTime::currentDateTime();
    auto it = sessions.begin();
    while (it != sessions.end()) {
        QDateTime lastSeen = it.value()->getLastSeen();
        qint64 hoursSinceLastSeen = lastSeen.secsTo(currentTime) / 3600;
        if (hoursSinceLastSeen > 24) {
            qDebug() << "Удаление просроченной сессии для пользователя:" << it.value()->getUserName();
            delete it.value();
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
    saveUsers();
}

AuthManager& AuthManager::operator+(User* user) {
    if (user && user->isValid()) {
        QString token = QString("%1_%2").arg(user->getId()).arg(QDateTime::currentMSecsSinceEpoch());
        sessions[token] = user;
        saveUsers();
    } else {
        qWarning() << "Попытка добавления невалидного пользователя";
    }
    return *this;
}

AuthManager& AuthManager::operator-(user_id user_id) {
    auto it = sessions.begin();
    while (it != sessions.end()) {
        if (it.value()->getId() == user_id) {
            delete it.value();
            it = sessions.erase(it);
            saveUsers();
            break;
        } else {
            ++it;
        }
    }
    return *this;
}

void AuthManager::saveUsers() {
    settings.beginWriteArray("sessions");
    int index = 0;
    for (auto it = sessions.begin(); it != sessions.end(); ++it) {
        settings.setArrayIndex(index);
        settings.setValue("token", it.key());
        settings.setValue("user_id", QVariant::fromValue(it.value()->getId()));
        settings.setValue("username", it.value()->getUserName());
        settings.setValue("password", it.value()->getPassword());
        settings.setValue("online", it.value()->isOnline());
        
        qint64 lastSeenMs = it.value()->getLastSeen().toMSecsSinceEpoch();
        settings.setValue("lastSeen", lastSeenMs);
        index++;
    }
    settings.endArray();
}

void AuthManager::loadUsers() {
    int size = settings.beginReadArray("sessions");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString token = settings.value("token").toString();
        user_id id = settings.value("user_id").toLongLong();
        QString username = settings.value("username").toString();
        QString password = settings.value("password").toString();
        
        if (token.isEmpty() || id == 0 || username.isEmpty()) {
            qWarning() << "Пропуск невалидной сохраненной сессии";
            continue;
        }
        
        User* user = new User(id, username, password);
        user->setOnline(settings.value("online").toBool());
        
        qint64 lastSeenMs = settings.value("lastSeen").toLongLong();
        QDateTime lastSeen = QDateTime::fromMSecsSinceEpoch(lastSeenMs);
        user->setLastSeen(lastSeen);
        
        sessions[token] = user;
    }
    settings.endArray();
}