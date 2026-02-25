#include "managers/message_manager.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>

MessageManager::MessageManager(QVector<Contact>& contacts,
                              QVector<std::shared_ptr<Chat>>& chats,
                              QVector<std::shared_ptr<Message<std::string>>>& messages,
                              QObject* parent)
    : QObject(parent)
    , m_contacts(contacts)
    , m_chats(chats)
    , m_messages(messages)
{
}

QJsonObject MessageManager::handleRequest(const QString& method, const QString& path, const QJsonObject& data)
{
    QJsonObject response;
    
    if (method == "GET") {
        if (path == "/contacts") {
            response = handleGetContacts();
        } else if (path == "/chats") {
            response = handleGetChats();
        } else if (path == "/messages") {
            response = handleGetMessages();
        } else {
            response["error"] = "Unknown endpoint";
        }
    } else if (method == "POST") {
        if (path == "/contacts") {
            response = handlePostContacts(data);
        } else if (path == "/chats") {
            response = handlePostChats(data);
        } else if (path == "/messages") {
            response = handlePostMessages(data);
        } else {
            response["error"] = "Unknown endpoint";
        }
    } else {
        response["error"] = "Method not supported";
    }
    
    bool success = !response.contains("error");
    emit requestProcessed(method, path, success);
    
    return response;
}

bool MessageManager::addContact(const QJsonObject& data)
{
    if (!data.contains("ownerId") || !data.contains("contactId") || !data.contains("contactName")) {
        return false;
    }
    
    contact_id id = m_contacts.isEmpty() ? 1 : m_contacts.last().id + 1;
    user_id ownerId = data["ownerId"].toInt();
    user_id contactId = data["contactId"].toInt();
    QString contactName = data["contactName"].toString();
    
    Contact newContact(id, ownerId, contactId, contactName);
    m_contacts.append(newContact);
    
    emit contactAdded(newContact);
    return true;
}

bool MessageManager::removeContact(contact_id id)
{
    for (int i = 0; i < m_contacts.size(); i++) {
        if (m_contacts[i].id == id) {
            m_contacts.remove(i);
            emit contactRemoved(id);
            return true;
        }
    }
    return false;
}

bool MessageManager::addChat(const QJsonObject& data)
{
    if (!data.contains("type") || !data.contains("participants")) {
        return false;
    }
    
    chat_id id = m_chats.isEmpty() ? 1 : m_chats.last()->id + 1;
    chat_type type = static_cast<chat_type>(data["type"].toInt());
    
    QJsonArray participantsArray = data["participants"].toArray();
    std::vector<user_id> participants;
    for (const auto& participant : participantsArray) {
        participants.push_back(participant.toInt());
    }
    
    auto newChat = std::make_shared<Chat>(id, type, participants);
    m_chats.append(newChat);
    
    emit chatAdded(newChat);
    return true;
}

bool MessageManager::removeChat(chat_id id)
{
    for (int i = 0; i < m_chats.size(); i++) {
        if (m_chats[i]->id == id) {
            m_chats.remove(i);
            emit chatRemoved(id);
            return true;
        }
    }
    return false;
}

bool MessageManager::addMessage(const QJsonObject& data)
{
    if (!data.contains("sender_id") || !data.contains("content")) {
        return false;
    }

    message_type msgType = static_cast<message_type>(data["type"].toInt(MESSAGE_NORMAL));
    chat_id receiver_id = data["receiver_id"].toInt(0);

    if (msgType == MESSAGE_NORMAL && !data.contains("receiver_id")) {
        return false;
    }

    message_id id = m_messages.isEmpty() ? 1 : m_messages.last()->id + 1;
    user_id sender_id = data["sender_id"].toInt();
    std::string content = data["content"].toString().toStdString();

    if (msgType == MESSAGE_BROADCAST) {
        receiver_id = -1;  // Рассылка во все чаты
    }

    auto newMessage = std::make_shared<Message<std::string>>(id, sender_id, receiver_id, content, msgType);
    m_messages.append(newMessage);

    if (msgType == MESSAGE_BROADCAST) {
        for (auto& chat : m_chats) {
            chat->addMessage(id);
        }
    } else {
        for (auto& chat : m_chats) {
            if (chat->id == receiver_id) {
                chat->addMessage(id);
                break;
            }
        }
    }

    emit messageAdded(newMessage);
    return true;
}

bool MessageManager::removeMessage(message_id id)
{
    for (int i = 0; i < m_messages.size(); i++) {
        if (m_messages[i]->id == id) {
            bool isBroadcast = (m_messages[i]->type == MESSAGE_BROADCAST);
            chat_id chatId = m_messages[i]->receiver_id;
            m_messages.remove(i);
            if (isBroadcast) {
                for (auto& chat : m_chats) {
                    chat->removeMessage(id);
                }
            } else {
                for (auto& chat : m_chats) {
                    if (chat->id == chatId) {
                        chat->removeMessage(id);
                        break;
                    }
                }
            }
            emit messageRemoved(id);
            return true;
        }
    }
    return false;
}

QJsonObject MessageManager::handleGetContacts()
{
    QJsonObject response;
    QJsonArray contactsArray;
    
    for (const auto& contact : m_contacts) {
        QJsonObject contactObj;
        contactObj["id"] = static_cast<qint64>(contact.id);
        contactObj["ownerId"] = static_cast<qint64>(contact.ownerId);
        contactObj["contactId"] = static_cast<qint64>(contact.contactId);
        contactObj["contactName"] = contact.contactName;
        contactObj["addedDate"] = contact.addedDate.toString(Qt::ISODate);
        
        contactsArray.append(contactObj);
    }
    
    response["contacts"] = contactsArray;
    response["count"] = m_contacts.size();
    return response;
}

QJsonObject MessageManager::handlePostContacts(const QJsonObject& data)
{
    QJsonObject response;
    
    if (addContact(data)) {
        response["status"] = "success";
        response["message"] = "Contact added successfully";
    } else {
        response["status"] = "error";
        response["message"] = "Failed to add contact";
    }
    
    return response;
}

QJsonObject MessageManager::handleGetChats()
{
    QJsonObject response;
    QJsonArray chatsArray;
    
    for (const auto& chat : m_chats) {
        QJsonObject chatObj;
        chatObj["id"] = static_cast<qint64>(chat->id);
        chatObj["type"] = static_cast<int>(chat->type);
        
        QJsonArray participantsArray;
        auto participants = chat->getParticipants();
        for (const auto& participant : participants) {
            participantsArray.append(static_cast<qint64>(participant));
        }
        chatObj["participants"] = participantsArray;
        
        QJsonArray messagesArray;
        auto messages = chat->getMessages();
        for (const auto& message : messages) {
            messagesArray.append(static_cast<qint64>(message));
        }
        chatObj["messages"] = messagesArray;
        
        chatsArray.append(chatObj);
    }
    
    response["chats"] = chatsArray;
    response["count"] = m_chats.size();
    return response;
}

QJsonObject MessageManager::handlePostChats(const QJsonObject& data)
{
    QJsonObject response;
    
    if (addChat(data)) {
        response["status"] = "success";
        response["message"] = "Chat added successfully";
    } else {
        response["status"] = "error";
        response["message"] = "Failed to add chat";
    }
    
    return response;
}

QJsonObject MessageManager::handleGetMessages()
{
    QJsonObject response;
    QJsonArray messagesArray;
    
    for (const auto& message : m_messages) {
        QJsonObject messageObj;
        messageObj["id"] = static_cast<qint64>(message->id);
        messageObj["sender_id"] = static_cast<qint64>(message->sender_id);
        messageObj["receiver_id"] = static_cast<qint64>(message->receiver_id);
        messageObj["type"] = static_cast<int>(message->type);
        messageObj["content"] = QString::fromStdString(message->getContent());
        messageObj["status"] = static_cast<int>(message->getStatus());
        
        auto timestamp = message->timestamp;
        auto duration = timestamp.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        QDateTime qtime = QDateTime::fromMSecsSinceEpoch(milliseconds);
        messageObj["timestamp"] = qtime.toString(Qt::ISODateWithMs);
        
        messagesArray.append(messageObj);
    }
    
    response["messages"] = messagesArray;
    response["count"] = m_messages.size();
    return response;
}

QJsonObject MessageManager::handlePostMessages(const QJsonObject& data)
{
    QJsonObject response;
    
    if (addMessage(data)) {
        response["status"] = "success";
        response["message"] = "Message added successfully";
    } else {
        response["status"] = "error";
        response["message"] = "Failed to add message";
    }
    
    return response;
}

QVector<Contact> MessageManager::getContacts() const
{
    return m_contacts;
}

QVector<std::shared_ptr<Chat>> MessageManager::getChats() const
{
    return m_chats;
}

QVector<std::shared_ptr<Message<std::string>>> MessageManager::getMessages() const
{
    return m_messages;
}