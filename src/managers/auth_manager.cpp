// auth_manager.cpp
#include "managers/auth_manager.h"
#include <QNetworkRequest>
#include <QJsonArray>
#include <QDateTime>

AuthManager::AuthManager(const QString& serverUrl) 
    : serverBaseUrl(serverUrl), settings("MINM", "AuthManager") {
    networkManager = new QNetworkAccessManager(this);
    
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AuthManager::handleLoginReply);
    
    loadUsers();
}

AuthManager::~AuthManager() {
    for (auto& session : sessions) {
        delete session.second;
    }
    sessions.clear();
}

void AuthManager::login(const std::string& username, const std::string& password) {
    QUrl url(serverBaseUrl + "/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["username"] = QString::fromStdString(username);
    json["password"] = QString::fromStdString(password);
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    
    disconnect(networkManager, &QNetworkAccessManager::finished,
               this, &AuthManager::handleLoginReply);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AuthManager::handleLoginReply);
    
    networkManager->post(request, data);
}

void AuthManager::logout(user_id user_id) {
    std::string sessionToken;
    for (const auto& session : sessions) {
        if (session.second->getId() == user_id) {
            sessionToken = session.first;
            break;
        }
    }
    
    if (sessionToken.empty()) {
        emit logoutFailed(user_id, "User not logged in");
        return;
    }
    
    QUrl url(serverBaseUrl + "/auth/logout");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + sessionToken).c_str());
    
    disconnect(networkManager, &QNetworkAccessManager::finished,
               this, &AuthManager::handleLogoutReply);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AuthManager::handleLogoutReply);
    
    networkManager->post(request, QByteArray());
}

void AuthManager::registerUser(const std::string& username, const std::string& password) {
    QUrl url(serverBaseUrl + "/auth/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["username"] = QString::fromStdString(username);
    json["password"] = QString::fromStdString(password);
    
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
        QJsonObject json = doc.object();
        
        if (json["success"].toBool()) {
            User* user = parseUserFromJson(json["user"].toObject());
            std::string token = json["token"].toString().toStdString();
            
            sessions[token] = user;
            saveUsers();
            emit loginSuccess(user);
        } else {
            emit loginFailed(json["error"].toString());
        }
    } else {
        emit loginFailed(reply->errorString());
    }
    
    reply->deleteLater();
}

void AuthManager::handleRegistrationReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject json = doc.object();
        
        if (json["success"].toBool()) {
            User* user = parseUserFromJson(json["user"].toObject());
            std::string token = json["token"].toString().toStdString();
            
            sessions[token] = user;
            saveUsers();
            emit registrationSuccess(user);
        } else {
            emit registrationFailed(json["error"].toString());
        }
    } else {
        emit registrationFailed(reply->errorString());
    }
    
    reply->deleteLater();
}

void AuthManager::handleLogoutReply(QNetworkReply* reply) {
    user_id user_id = reply->property("user_id").toLongLong();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject json = doc.object();
        
        if (json["success"].toBool()) {
            std::string sessionToken;
            for (auto it = sessions.begin(); it != sessions.end(); ++it) {
                if (it->second->getId() == user_id) {
                    sessionToken = it->first;
                    delete it->second;
                    sessions.erase(it);
                    saveUsers();
                    break;
                }
            }
            emit logoutSuccess(user_id);
        } else {
            emit logoutFailed(user_id, json["error"].toString());
        }
    } else {
        emit logoutFailed(user_id, reply->errorString());
    }
    
    reply->deleteLater();
}

User* AuthManager::parseUserFromJson(const QJsonObject& json) {
    user_id id = json["id"].toVariant().toLongLong();
    std::string username = json["username"].toString().toStdString();
    std::string password = json["password"].toString().toStdString();
    
    User* user = new User(id, username, password);
    user->setOnline(json["online"].toBool());
    
    qint64 lastSeenMs = json["lastSeen"].toVariant().toLongLong();
    auto lastSeen = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(lastSeenMs));
    user->setLastSeen(lastSeen);
    
    QJsonArray contactsArray = json["contacts"].toArray();
    std::vector<Contact> contacts;
    for (const QJsonValue& contactValue : contactsArray) {
        contacts.push_back(parseContactFromJson(contactValue.toObject()));
    }
    user->setContacts(contacts);
    
    QJsonArray blockedArray = json["blockedUsers"].toArray();
    std::vector<Contact> blockedUsers;
    for (const QJsonValue& blockedValue : blockedArray) {
        blockedUsers.push_back(parseContactFromJson(blockedValue.toObject()));
    }
    user->setBlockedUsers(blockedUsers);
    
    return user;
}

Contact AuthManager::parseContactFromJson(const QJsonObject& json) {
    contact_id id = json["id"].toVariant().toLongLong();
    user_id ownerId = json["ownerId"].toVariant().toLongLong();
    user_id contactId = json["contactId"].toVariant().toLongLong();
    std::string contactName = json["contactName"].toString().toStdString();
    
    qint64 addedDateMs = json["addedDate"].toVariant().toLongLong();
    auto addedDate = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(addedDateMs));
    
    return Contact(id, ownerId, contactId, contactName, addedDate);
}

bool AuthManager::isUserLoggedIn(user_id user_id) const {
    for (const auto& session : sessions) {
        if (session.second->getId() == user_id) {
            return true;
        }
    }
    return false;
}

void AuthManager::clearExpiredSessions() {
    auto currentTime = std::chrono::system_clock::now();
    auto it = sessions.begin();
    while (it != sessions.end()) {
        auto lastSeen = it->second->getLastSeen();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(currentTime - lastSeen);
        if (duration.count() > 24) {
            delete it->second;
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
    saveUsers();
}

AuthManager& AuthManager::operator+(User* user) {
    std::string token = std::to_string(user->getId()) + "_" + std::to_string(QDateTime::currentMSecsSinceEpoch());
    sessions[token] = user;
    saveUsers();
    return *this;
}

AuthManager& AuthManager::operator-(user_id user_id) {
    auto it = sessions.begin();
    while (it != sessions.end()) {
        if (it->second->getId() == user_id) {
            delete it->second;
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
    for (const auto& session : sessions) {
        settings.setArrayIndex(index);
        settings.setValue("token", QString::fromStdString(session.first));
        settings.setValue("user_id", QVariant::fromValue(session.second->getId()));
        settings.setValue("username", QString::fromStdString(session.second->getUserName()));
        settings.setValue("password", QString::fromStdString(session.second->getPassword()));
        settings.setValue("online", session.second->isOnline());
        
        qint64 lastSeenMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            session.second->getLastSeen().time_since_epoch()).count();
        settings.setValue("lastSeen", lastSeenMs);
        index++;
    }
    settings.endArray();
}

void AuthManager::loadUsers() {
    int size = settings.beginReadArray("sessions");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        std::string token = settings.value("token").toString().toStdString();
        user_id id = settings.value("user_id").toLongLong();
        std::string username = settings.value("username").toString().toStdString();
        std::string password = settings.value("password").toString().toStdString();
        
        User* user = new User(id, username, password);
        user->setOnline(settings.value("online").toBool());
        
        qint64 lastSeenMs = settings.value("lastSeen").toLongLong();
        auto lastSeen = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(lastSeenMs));
        user->setLastSeen(lastSeen);
        
        sessions[token] = user;
    }
    settings.endArray();
}