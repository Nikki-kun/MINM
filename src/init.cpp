#include "managers/message_manager.h"
#include "http_server.h"
#include "widget_manager.h"
#include <QApplication>
#include <QCommandLineParser>
#include <iostream>
#include <memory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Messenger HTTP Server");
    app.setApplicationVersion("1.0");
    
    QCommandLineParser parser;
    parser.setApplicationDescription("HTTP Server for Messenger Application");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption portOption("p", "Port to listen on", "port", "8080");
    parser.addOption(portOption);
    
    QCommandLineOption guiOption("g", "Enable GUI interface");
    parser.addOption(guiOption);
    
    parser.process(app);
    
    quint16 port = parser.value(portOption).toUShort();
    bool enableGUI = true;
    
    QVector<Contact> contacts;
    QVector<std::shared_ptr<Chat>> chats;
    QVector<std::shared_ptr<Message<std::string>>> messages;
    
    MessageManager manager(contacts, chats, messages);
    
    HttpServer server(manager);
    
    if (!server.start(port)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    WidgetManager *widgetManager = nullptr;
    
    if (enableGUI) {
        widgetManager = new WidgetManager(manager, contacts, chats, messages);
        widgetManager->show();
        QObject::connect(&manager, &MessageManager::contactAdded, widgetManager, &WidgetManager::onContactAdded);
        QObject::connect(&manager, &MessageManager::contactRemoved, widgetManager, &WidgetManager::onContactRemoved);
        QObject::connect(&manager, &MessageManager::chatAdded, widgetManager, &WidgetManager::onChatAdded);
        QObject::connect(&manager, &MessageManager::chatRemoved, widgetManager, &WidgetManager::onChatRemoved);
        QObject::connect(&manager, &MessageManager::messageAdded, widgetManager, &WidgetManager::onMessageAdded);
        QObject::connect(&manager, &MessageManager::messageRemoved, widgetManager, &WidgetManager::onMessageRemoved);
    }
    
    std::cout << "===============================================" << std::endl;
    std::cout << "Messenger HTTP Server running on port " << port << std::endl;
    if (enableGUI) {
        std::cout << "GUI interface enabled" << std::endl;
    }
    std::cout << "===============================================" << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  GET  /contacts" << std::endl;
    std::cout << "  POST /contacts" << std::endl;
    std::cout << "  GET  /chats" << std::endl;
    std::cout << "  POST /chats" << std::endl;
    std::cout << "  GET  /messages" << std::endl;
    std::cout << "  POST /messages" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    int result = app.exec();
    
    if (widgetManager) {
        delete widgetManager;
    }
    
    return result;
}