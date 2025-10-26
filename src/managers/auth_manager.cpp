#include "managers/auth_manager.h"
#include <QNetworkRequest>
#include <QJsonArray>
#include <QDateTime>

AuthManager::AuthManager(const QString& serverUrl) 
    : serverBaseUrl(serverUrl) {
    networkManager = new QNetworkAccessManager(this);
    
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AuthManager::handleLoginReply);
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
    // This would typically check session expiration times
    // For now, we'll keep all sessions until logout
}