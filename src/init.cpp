#include <QCoreApplication>
#include <QHttpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

#include "core/message.h"
#include "core/chat.h"
#include "core/contact.h"
#include "core/user.h"
#include "message_manager.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "Starting chat server...";
    
    // Хранилища в памяти (будут существовать только во время сессии)
    QHash<chat_id, std::shared_ptr<Chat>> chats;
    QHash<contact_id, std::shared_ptr<Contact>> contacts;
    QHash<user_id, std::shared_ptr<User>> users;
    
    // Создаем несколько тестовых пользователей для демонстрации
    auto user1 = std::make_shared<User>(1, "alice", "pass123");
    auto user2 = std::make_shared<User>(2, "bob", "pass456");
    auto user3 = std::make_shared<User>(3, "charlie", "pass789");
    
    users.insert(1, user1);
    users.insert(2, user2);
    users.insert(3, user3);
    
    qDebug() << "Created test users: Alice(1), Bob(2), Charlie(3)";
    
    // Создание менеджера сообщений
    MessageManager messageManager;
    messageManager.setChats(&chats);
    messageManager.setContacts(&contacts);
    messageManager.setUsers(&users);
    
    // Подключаем сигналы для вывода в терминал
    QObject::connect(&messageManager, &MessageManager::messageReceived,
        [](message_id msgId, user_id senderId, chat_id chatId, const QString& content) {
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] Message received! ID:" << msgId 
                     << "Sender:" << senderId 
                     << "Chat:" << chatId 
                     << "Content:" << content;
        });
    
    QObject::connect(&messageManager, &MessageManager::chatCreated,
        [](chat_id chatId, chat_type type, const std::vector<user_id>& participants) {
            QString typeStr;
            switch(type) {
                case PRIVATE: typeStr = "Private"; break;
                case GROUP: typeStr = "Group"; break;
                case CHANNEL: typeStr = "Channel"; break;
            }
            
            QString participantsStr;
            for (auto userId : participants) {
                participantsStr += QString::number(userId) + " ";
            }
            
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] Chat created! ID:" << chatId 
                     << "Type:" << typeStr
                     << "Participants:" << participantsStr;
        });
    
    QObject::connect(&messageManager, &MessageManager::contactCreated,
        [](contact_id contactId, user_id ownerId, user_id contactUserId, const QString& contactName) {
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] Contact created! ID:" << contactId 
                     << "Owner:" << ownerId 
                     << "Contact User:" << contactUserId
                     << "Name:" << contactName;
        });
    
    // Настройка HTTP-сервера
    QHttpServer server;
    
    // Маршрутизация запросов
    server.route("/message/send", QHttpServerRequest::Method::Post, 
        [&messageManager](const QHttpServerRequest &request) {
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] POST /message/send from:" << request.remoteAddress().toString();
            
            auto body = request.body();
            QJsonDocument doc = QJsonDocument::fromJson(body);
            
            if (doc.isNull() || !doc.isObject()) {
                qDebug() << "  Error: Invalid JSON";
                return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
            }
            
            QJsonObject json = doc.object();
            qDebug() << "  Request data:" << json;
            
            // В реальном приложении здесь был бы вызов messageManager.handleRequest
            // Для простоты сразу возвращаем ответ
            QJsonObject response;
            response["status"] = "success";
            response["messageId"] = 1001;
            response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            return QHttpServerResponse(QJsonDocument(response).toJson(), 
                                      "application/json");
        });
    
    server.route("/chat/create", QHttpServerRequest::Method::Post,
        [&messageManager](const QHttpServerRequest &request) {
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] POST /chat/create from:" << request.remoteAddress().toString();
            
            auto body = request.body();
            QJsonDocument doc = QJsonDocument::fromJson(body);
            
            if (doc.isNull() || !doc.isObject()) {
                qDebug() << "  Error: Invalid JSON";
                return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
            }
            
            QJsonObject json = doc.object();
            qDebug() << "  Request data:" << json;
            
            QJsonObject response;
            response["status"] = "success";
            response["chatId"] = 5001;
            response["createdDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            return QHttpServerResponse(QJsonDocument(response).toJson(), 
                                      "application/json");
        });
    
    server.route("/contact/create", QHttpServerRequest::Method::Post,
        [&messageManager](const QHttpServerRequest &request) {
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] POST /contact/create from:" << request.remoteAddress().toString();
            
            auto body = request.body();
            QJsonDocument doc = QJsonDocument::fromJson(body);
            
            if (doc.isNull() || !doc.isObject()) {
                qDebug() << "  Error: Invalid JSON";
                return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
            }
            
            QJsonObject json = doc.object();
            qDebug() << "  Request data:" << json;
            
            QJsonObject response;
            response["status"] = "success";
            response["contactId"] = 2001;
            response["addedDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            return QHttpServerResponse(QJsonDocument(response).toJson(), 
                                      "application/json");
        });
    
    server.route("/chats", QHttpServerRequest::Method::Get,
        [&chats](const QHttpServerRequest &request) {
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] GET /chats from:" << request.remoteAddress().toString();
            
            QJsonArray chatsArray;
            for (auto it = chats.begin(); it != chats.end(); ++it) {
                QJsonObject chatObj;
                chatObj["id"] = static_cast<int>(it.key());
                chatsArray.append(chatObj);
            }
            
            QJsonObject response;
            response["chats"] = chatsArray;
            response["count"] = chatsArray.size();
            
            qDebug() << "  Returning" << chatsArray.size() << "chats";
            
            return QHttpServerResponse(QJsonDocument(response).toJson(), 
                                      "application/json");
        });
    
    server.route("/contacts", QHttpServerRequest::Method::Get,
        [&contacts](const QHttpServerRequest &request) {
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] GET /contacts from:" << request.remoteAddress().toString();
            
            QJsonArray contactsArray;
            for (auto it = contacts.begin(); it != contacts.end(); ++it) {
                QJsonObject contactObj;
                contactObj["id"] = static_cast<int>(it.key());
                contactsArray.append(contactObj);
            }
            
            QJsonObject response;
            response["contacts"] = contactsArray;
            response["count"] = contactsArray.size();
            
            qDebug() << "  Returning" << contactsArray.size() << "contacts";
            
            return QHttpServerResponse(QJsonDocument(response).toJson(), 
                                      "application/json");
        });
    
    // Корневой маршрут с информацией о сервере
    server.route("/", QHttpServerRequest::Method::Get,
        [](const QHttpServerRequest &request) {
            qDebug() << "[" << QDateTime::currentDateTime().toString("hh:mm:ss") 
                     << "] GET / from:" << request.remoteAddress().toString();
            
            QJsonObject info;
            info["server"] = "Chat Server";
            info["version"] = "1.0";
            info["endpoints"] = QJsonArray::fromStringList({
                "POST /message/send",
                "POST /chat/create", 
                "POST /contact/create",
                "GET /chats",
                "GET /contacts"
            });
            info["time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            return QHttpServerResponse(QJsonDocument(info).toJson(), 
                                      "application/json");
        });
    
    // Запуск сервера
    const int port = 8080;
    if (!server.listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start server on port" << port;
        return -1;
    }
    
    qDebug() << "===========================================";
    qDebug() << "Chat Server started successfully!";
    qDebug() << "Server URL: http://localhost:" << port;
    qDebug() << "Endpoints:";
    qDebug() << "  POST http://localhost:" << port << "/message/send";
    qDebug() << "  POST http://localhost:" << port << "/chat/create";
    qDebug() << "  POST http://localhost:" << port << "/contact/create";
    qDebug() << "  GET  http://localhost:" << port << "/chats";
    qDebug() << "  GET  http://localhost:" << port << "/contacts";
    qDebug() << "===========================================";
    qDebug() << "Use Ctrl+C to stop the server";
    qDebug() << "";
    
    // Пример тестового запроса через 2 секунды
    QTimer::singleShot(2000, []() {
        qDebug() << "";
        qDebug() << "===========================================";
        qDebug() << "Test commands to try (using curl):";
        qDebug() << "  Send message:";
        qDebug() << "    curl -X POST http://localhost:8080/message/send \\";
        qDebug() << "      -H \"Content-Type: application/json\" \\";
        qDebug() << "      -d '{\"userId\":1,\"chatId\":100,\"content\":\"Hello world!\"}'";
        qDebug() << "";
        qDebug() << "  Create chat:";
        qDebug() << "    curl -X POST http://localhost:8080/chat/create \\";
        qDebug() << "      -H \"Content-Type: application/json\" \\";
        qDebug() << "      -d '{\"type\":\"GROUP\",\"participants\":[1,2,3]}'";
        qDebug() << "";
        qDebug() << "  Create contact:";
        qDebug() << "    curl -X POST http://localhost:8080/contact/create \\";
        qDebug() << "      -H \"Content-Type: application/json\" \\";
        qDebug() << "      -d '{\"ownerId\":1,\"contactUserId\":2,\"contactName\":\"Bob\"}'";
        qDebug() << "===========================================";
    });
    
    return app.exec();
}