#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QThread>
#include <memory>
#include "managers/message_manager.h"

class RequestProcessor;

class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(MessageManager& manager, QObject* parent = nullptr);
    ~HttpServer();

    bool start(quint16 port = 8080);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    struct ClientData {
        QByteArray buffer;
        qintptr socketDescriptor;

        ClientData() : socketDescriptor(0) {}
        ClientData(qintptr descriptor) : socketDescriptor(descriptor) {}
    };

    void sendHttpResponse(qintptr socketDescriptor, int statusCode, 
                         const QString& statusText, 
                         const QString& contentType, 
                         const QByteArray& body);
    void sendHttpResponse(qintptr socketDescriptor, int statusCode, 
                         const QString& statusText, 
                         const QJsonObject& jsonData);
    
    RequestProcessor* m_requestProcessor;
    QThread* m_processorThread;
    MessageManager& m_manager;
    
    QTcpServer m_server;
    QMap<qintptr, ClientData> m_clients;
    QMap<qintptr, QTcpSocket*> m_sockets;
};

Q_DECLARE_METATYPE(qintptr)

#endif