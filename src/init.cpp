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
                     qDebug() << "[Contact Added Thread:" << QThread::currentThread() 
                              << "]" << contact.contactName;
                 });

QObject::connect(&manager, &MessageManager::chatAdded,
                 [](std::shared_ptr<Chat> chat) {
                     qDebug() << "[Chat Created Thread:" << QThread::currentThread() 
                              << "] ID:" << chat->id;
                 });

QObject::connect(&manager, &MessageManager::messageAdded,
                 [](std::shared_ptr<Message<std::string>> message) {
                     qDebug() << "[Message Sent Thread:" << QThread::currentThread() 
                              << "] From:" << message->sender_id;
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