#ifndef MESSAGE_MANAGER_H
#define MESSAGE_MANAGER_H

#include "../core/message.h"
#include "../core/сhat.h"
#include "../core/contact.h"
#include "../core/user.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QHash>
#include <QString>
#include <QObject>
#include <memory>
#include <functional>

class MessageManager : public QObject {
    Q_OBJECT

public:
    explicit MessageManager(QObject* parent = nullptr);
    
    // Установка ссылок на массивы чатов и контактов из main
    void setChats(QHash<chat_id, std::shared_ptr<Chat>>* chats);
    void setContacts(QHash<contact_id, std::shared_ptr<Contact>>* contacts);
    void setUsers(QHash<user_id, std::shared_ptr<User>>* users);
    
    // Обработка входящих HTTP-запросов
    void handleRequest(const QNetworkRequest& request, QNetworkReply* reply);

private:
    QNetworkAccessManager* networkManager;
    QHash<chat_id, std::shared_ptr<Chat>>* chats;      // Ссылка на чаты из main
    QHash<contact_id, std::shared_ptr<Contact>>* contacts; // Ссылка на контакты из main
    QHash<user_id, std::shared_ptr<User>>* users;      // Ссылка на пользователей из main
    
    // Обработчики для разных типов запросов
    void handleSendMessage(const QJsonObject& json, QNetworkReply* reply);
    void handleCreateChat(const QJsonObject& json, QNetworkReply* reply);
    void handleCreateContact(const QJsonObject& json, QNetworkReply* reply);
    void handleGetChats(QNetworkReply* reply);
    void handleGetContacts(QNetworkReply* reply);
    
    // Генерация ID
    message_id generateMessageId();
    chat_id generateChatId();
    contact_id generateContactId();
    
    // Отправка ответа клиенту
    void sendResponse(QNetworkReply* reply, const QJsonObject& json, int statusCode = 200);
    void sendError(QNetworkReply* reply, const QString& error, int statusCode = 400);

private slots:
    void onRequestReceived(QNetworkReply* reply);

signals:
    void messageReceived(message_id msgId, user_id senderId, chat_id chatId, const QString& content);
    void chatCreated(chat_id chatId, chat_type type, const std::vector<user_id>& participants);
    void contactCreated(contact_id contactId, user_id ownerId, user_id contactUserId, const QString& contactName);
};

#endif // MESSAGE_MANAGER_H