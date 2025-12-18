#include "managers/message_manager.h"
#include "http_server.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <iostream>
#include <memory>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("Messenger HTTP Server");
    app.setApplicationVersion("1.0");
    
    // Парсинг аргументов командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("HTTP Server for Messenger Application");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption portOption("p", "Port to listen on", "port", "8080");
    parser.addOption(portOption);
    
    parser.process(app);
    
    quint16 port = parser.value(portOption).toUShort();
    
    // Создаём коллекции
    QVector<Contact> contacts;
    QVector<std::shared_ptr<Chat>> chats;
    QVector<std::shared_ptr<Message<std::string>>> messages;
    
    // Создаём менеджер сообщений
    MessageManager manager(contacts, chats, messages);
    
    // Создаём HTTP сервер
    HttpServer server(manager);
    
    if (!server.start(port)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    // Подключаем обработчики сигналов
    QObject::connect(&manager, &MessageManager::contactAdded,
                     [](const Contact& contact) {
                         std::cout << "[Contact Added] " 
                                   << contact.contactName.toStdString() 
                                   << " (ID: " << contact.id << ")" << std::endl;
                     });
    
    QObject::connect(&manager, &MessageManager::chatAdded,
                     [](std::shared_ptr<Chat> chat) {
                         std::cout << "[Chat Created] ID: " 
                                   << chat->id 
                                   << " (Participants: " << chat->getParticipants().size() 
                                   << ")" << std::endl;
                     });
    
    QObject::connect(&manager, &MessageManager::messageAdded,
                     [](std::shared_ptr<Message<std::string>> message) {
                         std::cout << "[Message Sent] From: " 
                                   << message->sender_id 
                                   << " To Chat: " << message->receiver_id 
                                   << " Content: " << message->getContent().substr(0, 50) 
                                   << "..." << std::endl;
                     });
    
    QObject::connect(&manager, &MessageManager::requestProcessed,
                     [](const QString& method, const QString& path, bool success) {
                         std::cout << "[" << (success ? "SUCCESS" : "FAILED") << "] "
                                   << method.toStdString() << " " << path.toStdString() << std::endl;
                     });
    
    std::cout << "Messenger HTTP Server running on port " << port << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    return app.exec();
}