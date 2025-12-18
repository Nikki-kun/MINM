#include "managers/message_manager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <algorithm>
#include <random>

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
    
    qDebug() << "HTTP Request:" << method << path;
    
    if (method == "POST") {
        if (path == "/contacts") {
            response = handleAddContact(data);
        }
        else if (path == "/chats") {
            response = handleAddChat(data);
        }
        else if (path == "/messages") {
            response = handleAddMessage(data);
        }
        else {
            response["success"] = false;
            response["error"] = "Invalid endpoint";
        }
    }
    else if (method == "GET") {
        if (path == "/contacts") {
            response = handleGetContacts();
        }
        else if (path == "/chats") {
            response = handleGetChats();
        }
        else if (path == "/messages") {
            response = handleGetMessages();
        }
        else {
            response["success"] = false;
            response["error"] = "Invalid endpoint";
        }
    }
    else {
        response["success"] = false;
        response["error"] = "Method not allowed";
    }
    
    emit requestProcessed(method, path, response["success"].toBool());
    return response;
}

QJsonObject MessageManager::handleAddContact(const QJsonObject& data)
{
    QJsonObject response;
    
    if (!data.contains("owner_id") || !data.contains("contact_id") || !data.contains("name")) {
        response["success"] = false;
        response["error"] = "Missing required fields: owner_id, contact_id, name";
        return response;
    }
    
    user_id ownerId = data["owner_id"].toInt();
    user_id contactId = data["contact_id"].toInt();
    QString contactName = data["name"].toString();
    
    if (contactExists(ownerId, contactId)) {
        response["success"] = false;
        response["error"] = QString("Contact already exists").toStdString().c_str();
        return response;
    }
    
    if (!userExists(ownerId) || !userExists(contactId)) {
        response["success"] = false;
        response["error"] = "One or both users do not exist";
        return response;
    }
    
    contact_id newId = generateContactId();
    Contact newContact(newId, ownerId, contactId, contactName);
    
    m_contacts.append(newContact);
    
    response["success"] = true;
    response["contact_id"] = static_cast<int>(newId);
    response["message"] = "Contact added successfully";
    
    emit contactAdded(newContact);
    
    qDebug() << "[CONTACT ADDED] ID:" << newId
             << "Owner:" << ownerId
             << "Contact:" << contactId
             << "Name:" << contactName;
    
    return response;
}

QJsonObject MessageManager::handleAddChat(const QJsonObject& data)
{
    QJsonObject response;
    
    if (!data.contains("type") || !data.contains("participants")) {
        response["success"] = false;
        response["error"] = "Missing required fields: type, participants";
        return response;
    }
    
    chat_type type = static_cast<chat_type>(data["type"].toInt());
    QJsonArray participantsArray = data["participants"].toArray();
    
    std::vector<user_id> participants;
    for (const QJsonValue& participant : participantsArray) {
        participants.push_back(participant.toInt());
    }
    
    if (participants.empty()) {
        response["success"] = false;
        response["error"] = "At least one participant is required";
        return response;
    }
    
    for (user_id userId : participants) {
        if (!userExists(userId)) {
            response["success"] = false;
            response["error"] = QString("User %1 does not exist").arg(userId).toStdString().c_str();
            return response;
        }
    }
    
    chat_id newId = generateChatId();
    auto newChat = std::make_shared<Chat>(newId, type, participants);
    
    m_chats.append(newChat);
    
    response["success"] = true;
    response["chat_id"] = static_cast<int>(newId);
    response["message"] = "Chat created successfully";
    
    emit chatAdded(newChat);
    
    qDebug() << "[CHAT ADDED] ID:" << newId
             << "Type:" << type
             << "Participants:" << participants.size();
    
    return response;
}

QJsonObject MessageManager::handleAddMessage(const QJsonObject& data)
{
    QJsonObject response;
    
    if (!data.contains("sender_id") || !data.contains("chat_id") || !data.contains("content")) {
        response["success"] = false;
        response["error"] = "Missing required fields: sender_id, chat_id, content";
        return response;
    }
    
    user_id senderId = data["sender_id"].toInt();
    chat_id chatId = data["chat_id"].toInt();
    std::string content = data["content"].toString().toStdString();
    
    if (!userExists(senderId)) {
        response["success"] = false;
        response["error"] = QString("Sender %1 does not exist").arg(senderId).toStdString().c_str();
        return response;
    }
    
    auto chatIt = std::find_if(m_chats.begin(), m_chats.end(),
                               [chatId](const std::shared_ptr<Chat>& chat) { 
                                   return chat->id == chatId; 
                               });
    
    if (chatIt == m_chats.end()) {
        response["success"] = false;
        response["error"] = QString("Chat %1 does not exist").arg(chatId).toStdString().c_str();
        return response;
    }
    
    if (!(*chatIt)->hasParticipant(senderId)) {
        response["success"] = false;
        response["error"] = QString("Sender is not a participant of this chat").toStdString().c_str();
        return response;
    }
    
    message_id newId = generateMessageId();
    auto newMessage = std::make_shared<Message<std::string>>(newId, senderId, chatId, content);
    
    m_messages.append(newMessage);
    
    (*chatIt)->addMessage(newId);
    
    response["success"] = true;
    response["message_id"] = static_cast<int>(newId);
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    response["message"] = "Message sent successfully";
    
    emit messageAdded(newMessage);
    
    qDebug() << "[MESSAGE ADDED] ID:" << newId
             << "Sender:" << senderId
             << "Chat:" << chatId
             << "Content:" << QString::fromStdString(content).left(50) + "...";
    
    return response;
}

QJsonObject MessageManager::handleGetContacts()
{
    QJsonObject response;
    QJsonArray contactsArray;
    
    for (const Contact& contact : m_contacts) {
        QJsonObject contactObj;
        contactObj["id"] = static_cast<int>(contact.id);
        contactObj["owner_id"] = static_cast<int>(contact.ownerId);
        contactObj["contact_id"] = static_cast<int>(contact.contactId);
        contactObj["name"] = contact.contactName;
        contactObj["added_date"] = contact.addedDate.toString(Qt::ISODate);
        contactsArray.append(contactObj);
    }
    
    response["success"] = true;
    response["contacts"] = contactsArray;
    response["count"] = contactsArray.size();
    
    return response;
}

QJsonObject MessageManager::handleGetChats()
{
    QJsonObject response;
    QJsonArray chatsArray;
    
    for (const auto& chat : m_chats) {
        QJsonObject chatObj;
        chatObj["id"] = static_cast<int>(chat->id);
        chatObj["type"] = chat->type;
        
        QJsonArray participantsArray;
        for (user_id participant : chat->getParticipants()) {
            participantsArray.append(static_cast<int>(participant));
        }
        chatObj["participants"] = participantsArray;
        chatObj["message_count"] = static_cast<int>(chat->getMessages().size());
        chatObj["created_date"] = QDateTime::fromMSecsSinceEpoch(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                chat->created_date.time_since_epoch()).count()).toString(Qt::ISODate);
        
        chatsArray.append(chatObj);
    }
    
    response["success"] = true;
    response["chats"] = chatsArray;
    response["count"] = chatsArray.size();
    
    return response;
}

QJsonObject MessageManager::handleGetMessages()
{
    QJsonObject response;
    QJsonArray messagesArray;
    
    for (const auto& msg : m_messages) {
        QJsonObject msgObj;
        msgObj["id"] = static_cast<int>(msg->id);
        msgObj["sender_id"] = static_cast<int>(msg->sender_id);
        msgObj["chat_id"] = static_cast<int>(msg->receiver_id);
        msgObj["content"] = QString::fromStdString(msg->getContent());
        msgObj["status"] = msg->getStatus();
        msgObj["timestamp"] = QDateTime::fromMSecsSinceEpoch(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                msg->timestamp.time_since_epoch()).count()).toString(Qt::ISODate);
        
        messagesArray.append(msgObj);
    }
    
    response["success"] = true;
    response["messages"] = messagesArray;
    response["count"] = messagesArray.size();
    
    return response;
}

bool MessageManager::userExists(user_id userId)
{
    return userId >= 1 && userId <= 100;
}

bool MessageManager::contactExists(user_id ownerId, user_id contactId)
{
    auto it = std::find_if(m_contacts.begin(), m_contacts.end(),
                           [ownerId, contactId](const Contact& contact) {
                               return contact.ownerId == ownerId && contact.contactId == contactId;
                           });
    return it != m_contacts.end();
}

bool MessageManager::chatExists(chat_id id)
{
    auto it = std::find_if(m_chats.begin(), m_chats.end(),
                           [id](const std::shared_ptr<Chat>& chat) { 
                               return chat->id == id; 
                           });
    return it != m_chats.end();
}

bool MessageManager::messageExists(message_id id)
{
    auto it = std::find_if(m_messages.begin(), m_messages.end(),
                           [id](const std::shared_ptr<Message<std::string>>& msg) { 
                               return msg->id == id; 
                           });
    return it != m_messages.end();
}

contact_id MessageManager::generateContactId()
{
    static contact_id nextId = 1;
    return nextId++;
}

chat_id MessageManager::generateChatId()
{
    static chat_id nextId = 1000;
    return nextId++;
}

message_id MessageManager::generateMessageId()
{
    static message_id nextId = 10000;
    return nextId++;
}