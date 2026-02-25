#ifndef MESSAGE_MANAGER_H
#define MESSAGE_MANAGER_H

#include <QObject>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "core/contact.h"
#include "core/сhat.h"
#include "core/message.h"

class MessageManager : public QObject
{
    Q_OBJECT

public:
    MessageManager(QVector<Contact>& contacts,
                  QVector<std::shared_ptr<Chat>>& chats,
                  QVector<std::shared_ptr<Message<std::string>>>& messages,
                  QObject* parent = nullptr);
    
    QJsonObject handleRequest(const QString& method, const QString& path, const QJsonObject& data);
    
    bool addContact(const QJsonObject& data);
    bool removeContact(contact_id id);
    bool addChat(const QJsonObject& data);
    bool removeChat(chat_id id);
    bool addMessage(const QJsonObject& data);
    bool removeMessage(message_id id);
    
    QVector<Contact> getContacts() const;
    QVector<std::shared_ptr<Chat>> getChats() const;
    QVector<std::shared_ptr<Message<std::string>>> getMessages() const;

signals:
    void contactAdded(const Contact& contact);
    void contactRemoved(contact_id id);
    void chatAdded(std::shared_ptr<Chat> chat);
    void chatRemoved(chat_id id);
    void messageAdded(std::shared_ptr<Message<std::string>> message);
    void messageRemoved(message_id id);
    void requestProcessed(const QString& method, const QString& path, bool success);

private:
    QVector<Contact>& m_contacts;
    QVector<std::shared_ptr<Chat>>& m_chats;
    QVector<std::shared_ptr<Message<std::string>>>& m_messages;
    
    QJsonObject handleGetContacts();
    QJsonObject handlePostContacts(const QJsonObject& data);
    QJsonObject handleGetChats();
    QJsonObject handlePostChats(const QJsonObject& data);
    QJsonObject handleGetMessages();
    QJsonObject handlePostMessages(const QJsonObject& data);
};

#endif