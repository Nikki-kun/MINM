#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include "../core/user.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QHash>
#include <QString>

class AuthManager : public QObject {
    Q_OBJECT

private:
    QHash<QString, User*> sessions;
    QNetworkAccessManager* networkManager;
    QString serverBaseUrl;
    QSettings settings;

public:
    AuthManager(const QString& serverUrl);
    ~AuthManager();
    
    void login(const QString& username, const QString& password);
    void logout(user_id user_id);
    void registerUser(const QString& username, const QString& password);
    
    bool isUserLoggedIn(user_id user_id) const;
    void clearExpiredSessions();
    
    AuthManager& operator+(User* user);
    AuthManager& operator-(user_id user_id);

signals:
    void loginSuccess(User* user);
    void loginFailed(const QString& error);
    void registrationSuccess(User* user);
    void registrationFailed(const QString& error);
    void logoutSuccess(user_id user_id);
    void logoutFailed(user_id user_id, const QString& error);

private slots:
    void handleLoginReply(QNetworkReply* reply);
    void handleRegistrationReply(QNetworkReply* reply);
    void handleLogoutReply(QNetworkReply* reply);

private:
    User* parseUserFromJson(const QJsonObject& json);
    Contact parseContactFromJson(const QJsonObject& json);
    void saveUsers();
    void loadUsers();
};

#endif