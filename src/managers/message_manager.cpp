
// message_manager.cpp
#include "managers/message_manager.h"
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QRandomGenerator>
#include <QDateTime>

MessageManager::MessageManager(QObject* parent) 
    : QObject(parent), 
      networkManager(new QNetworkAccessManager(this)),
      chats(nullptr),
      contacts(nullptr),
      users(nullptr) {
    
    connect(networkManager, &QNetworkAccessManager::finished, 
            this, &MessageManager::onRequestReceived);
}

void MessageManager::setChats(QHash<chat_id, std::shared_ptr<Chat>>* chatsPtr) {
    chats = chatsPtr;
}

void MessageManager::setContacts(QHash<contact_id, std::shared_ptr<Contact>>* contactsPtr) {
    contacts = contactsPtr;
}

void MessageManager::setUsers(QHash<user_id, std::shared_ptr<User>>* usersPtr) {
    users = usersPtr;
}

void MessageManager::handleRequest(const QNetworkRequest& request, QNetworkReply* reply) {
    QString path = request.url().path();
    QString method = request.attribute(QNetworkRequest::CustomVerbAttribute).toString();
    
    // Чтение тела запроса
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (path == "/message/send" && method == "POST") {
        if (doc.isObject()) {
            handleSendMessage(doc.object(), reply);
        } else {
            sendError(reply, "Invalid JSON format");
        }
    } 
    else if (path == "/chat/create" && method == "POST") {
        if (doc.isObject()) {
            handleCreateChat(doc.object(), reply);
        } else {
            sendError(reply, "Invalid JSON format");
        }
    }
    else if (path == "/contact/create" && method == "POST") {
        if (doc.isObject()) {
            handleCreateContact(doc.object(), reply);
        } else {
            sendError(reply, "Invalid JSON format");
        }
    }
    else if (path == "/chats" && method == "GET") {
        handleGetChats(reply);
    }
    else if (path == "/contacts" && method == "GET") {
        handleGetContacts(reply);
    }
    else {
        sendError(reply, "Unknown endpoint", 404);
    }
}

void MessageManager::handleSendMessage(const QJsonObject& json, QNetworkReply* reply) {
    if (!json.contains("userId") || !json.contains("chatId") || !json.contains("content")) {
        sendError(reply, "Missing required fields: userId, chatId, content");
        return;
    }
    
    user_id userId = json["userId"].toInt();
    chat_id chatId = json["chatId"].toInt();
    QString content = json["content"].toString();
    
    // Проверка существования чата
    if (!chats || !chats->contains(chatId)) {
        sendError(reply, "Chat not found");
        return;
    }
    
    // Проверка, что пользователь является участником чата
    auto chat = chats->value(chatId);
    if (!chat->hasParticipant(userId)) {
        sendError(reply, "User is not a participant of this chat");
        return;
    }
    
    // Создание сообщения
    message_id msgId = generateMessageId();
    auto message = std::make_shared<Message<QString>>(
        msgId,
        userId,
        userId, // receiver_id - в чатах это chatId, но в текущей структуре нужен user_id
        content,
        std::chrono::system_clock::now()
    );
    
    // Добавление сообщения в чат
    chat->addMessage(msgId);
    
    // Отправка ответа
    QJsonObject response;
    response["messageId"] = static_cast<int>(msgId);
    response["senderId"] = static_cast<int>(userId);
    response["chatId"] = static_cast<int>(chatId);
    response["content"] = content;
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    response["status"] = "SENT";
    
    emit messageReceived(msgId, userId, chatId, content);
    sendResponse(reply, response);
}

void MessageManager::handleCreateChat(const QJsonObject& json, QNetworkReply* reply) {
    if (!json.contains("type") || !json.contains("participants")) {
        sendError(reply, "Missing required fields: type, participants");
        return;
    }
    
    QString typeStr = json["type"].toString();
    chat_type type = PRIVATE;
    
    if (typeStr == "GROUP") type = GROUP;
    else if (typeStr == "CHANNEL") type = CHANNEL;
    
    QJsonArray participantsArray = json["participants"].toArray();
    std::vector<user_id> participants;
    
    for (const auto& participant : participantsArray) {
        participants.push_back(participant.toInt());
    }
    
    // Проверка существования пользователей
    if (users) {
        for (auto userId : participants) {
            if (!users->contains(userId)) {
                sendError(reply, QString("User %1 not found").arg(userId));
                return;
            }
        }
    }
    
    // Создание чата
    chat_id chatId = generateChatId();
    auto chat = std::make_shared<Chat>(chatId, type, participants);
    
    // Сохранение чата
    if (chats) {
        chats->insert(chatId, chat);
    }
    
    // Отправка ответа
    QJsonObject response;
    response["chatId"] = static_cast<int>(chatId);
    response["type"] = typeStr;
    
    QJsonArray partArray;
    for (auto userId : participants) {
        partArray.append(static_cast<int>(userId));
    }
    response["participants"] = partArray;
    response["createdDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    emit chatCreated(chatId, type, participants);
    sendResponse(reply, response);
}

void MessageManager::handleCreateContact(const QJsonObject& json, QNetworkReply* reply) {
    if (!json.contains("ownerId") || !json.contains("contactUserId") || !json.contains("contactName")) {
        sendError(reply, "Missing required fields: ownerId, contactUserId, contactName");
        return;
    }
    
    user_id ownerId = json["ownerId"].toInt();
    user_id contactUserId = json["contactUserId"].toInt();
    QString contactName = json["contactName"].toString();
    
    // Проверка существования пользователей
    if (users && (!users->contains(ownerId) || !users->contains(contactUserId))) {
        sendError(reply, "User not found");
        return;
    }
    
    // Создание контакта
    contact_id contactId = generateContactId();
    auto contact = std::make_shared<Contact>(contactId, ownerId, contactUserId, contactName);
    
    // Сохранение контакта
    if (contacts) {
        contacts->insert(contactId, contact);
    }
    
    // Добавление контакта пользователю
    if (users && users->contains(ownerId)) {
        users->value(ownerId)->addContact(*contact);
    }
    
    // Отправка ответа
    QJsonObject response;
    response["contactId"] = static_cast<int>(contactId);
    response["ownerId"] = static_cast<int>(ownerId);
    response["contactUserId"] = static_cast<int>(contactUserId);
    response["contactName"] = contactName;
    response["addedDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    emit contactCreated(contactId, ownerId, contactUserId, contactName);
    sendResponse(reply, response);
}

void MessageManager::handleGetChats(QNetworkReply* reply) {
    QJsonArray chatsArray;
    
    if (chats) {
        for (auto it = chats->begin(); it != chats->end(); ++it) {
            QJsonObject chatObj;
            chatObj["chatId"] = static_cast<int>(it.key());
            chatObj["type"] = it.value()->type == PRIVATE ? "PRIVATE" : 
                              it.value()->type == GROUP ? "GROUP" : "CHANNEL";
            
            QJsonArray participants;
            auto partList = it.value()->getParticipants();
            for (auto userId : partList) {
                participants.append(static_cast<int>(userId));
            }
            chatObj["participants"] = participants;
            
            // Получение последних сообщений (например, последние 10)
            auto messages = it.value()->getMessages();
            QJsonArray messagesArray;
            int count = 0;
            for (auto itMsg = messages.rbegin(); itMsg != messages.rend() && count < 10; ++itMsg, ++count) {
                messagesArray.append(static_cast<int>(*itMsg));
            }
            chatObj["recentMessages"] = messagesArray;
            
            chatsArray.append(chatObj);
        }
    }
    
    QJsonObject response;
    response["chats"] = chatsArray;
    response["count"] = chatsArray.size();
    
    sendResponse(reply, response);
}

void MessageManager::handleGetContacts(QNetworkReply* reply) {
    QJsonArray contactsArray;
    
    if (contacts) {
        for (auto it = contacts->begin(); it != contacts->end(); ++it) {
            QJsonObject contactObj;
            contactObj["contactId"] = static_cast<int>(it.key());
            contactObj["ownerId"] = static_cast<int>(it.value()->ownerId);
            contactObj["contactUserId"] = static_cast<int>(it.value()->contactId);
            contactObj["contactName"] = it.value()->contactName;
            contactObj["addedDate"] = it.value()->addedDate.toString(Qt::ISODate);
            
            contactsArray.append(contactObj);
        }
    }
    
    QJsonObject response;
    response["contacts"] = contactsArray;
    response["count"] = contactsArray.size();
    
    sendResponse(reply, response);
}

message_id MessageManager::generateMessageId() {
    return QRandomGenerator::global()->generate() % 1000000 + 1;
}

chat_id MessageManager::generateChatId() {
    return QRandomGenerator::global()->generate() % 1000000 + 1;
}

contact_id MessageManager::generateContactId() {
    return QRandomGenerator::global()->generate() % 1000000 + 1;
}

void MessageManager::sendResponse(QNetworkReply* reply, const QJsonObject& json, int statusCode) {
    QJsonDocument doc(json);
    reply->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    reply->setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
    reply->write(doc.toJson());
    reply->close();
}

void MessageManager::sendError(QNetworkReply* reply, const QString& error, int statusCode) {
    QJsonObject errorObj;
    errorObj["error"] = error;
    errorObj["statusCode"] = statusCode;
    
    sendResponse(reply, errorObj, statusCode);
}

void MessageManager::onRequestReceived(QNetworkReply* reply) {
    // Этот слот будет вызываться при получении HTTP-запроса
    // В реальной реализации здесь будет логика маршрутизации запросов
    // В данном примере мы используем handleRequest напрямую
}