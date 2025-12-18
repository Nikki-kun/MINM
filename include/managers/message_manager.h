#ifndef MESSAGE_MANAGER_H
#define MESSAGE_MANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>
#include <functional>
#include "../core/message.h"
#include "../core/сhat.h"
#include "../core/contact.h"
#include "../core/user.h"

class MessageManager : public QObject {
    Q_OBJECT

public:
    MessageManager(QVector<Contact>& contacts,
                   QVector<std::shared_ptr<Chat>>& chats,
                   QVector<std::shared_ptr<Message<std::string>>>& messages,
                   QObject* parent = nullptr);

    QJsonObject handleRequest(const QString& method, const QString& path, const QJsonObject& data);

    signals:
    void contactAdded(const Contact& contact);
    void chatAdded(std::shared_ptr<Chat> chat);
    void messageAdded(std::shared_ptr<Message<std::string>> message);
    void requestProcessed(const QString& method, const QString& path, bool success);

private:
    QVector<Contact>& m_contacts;
    QVector<std::shared_ptr<Chat>>& m_chats;
    QVector<std::shared_ptr<Message<std::string>>>& m_messages;
    
    QJsonObject handleAddContact(const QJsonObject& data);
    QJsonObject handleAddChat(const QJsonObject& data);
    QJsonObject handleAddMessage(const QJsonObject& data);
    QJsonObject handleGetContacts();
    QJsonObject handleGetChats();
    QJsonObject handleGetMessages();
    
    bool userExists(user_id userId);
    bool contactExists(user_id ownerId, user_id contactId);
    bool chatExists(chat_id id);
    bool messageExists(message_id id);
    
    contact_id generateContactId();
    chat_id generateChatId();
    message_id generateMessageId();
};

#endif