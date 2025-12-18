#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include "managers/message_manager.h"

class HttpServer : public QObject {
    Q_OBJECT

public:
    explicit HttpServer(MessageManager& manager, QObject* parent = nullptr);
    bool start(quint16 port = 8080);
    
signals:
    void requestReceived(const QString& method, const QString& path, const QJsonObject& data);
    
private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    
private:
    QTcpServer m_server;
    MessageManager& m_manager;
    
    struct ClientData {
        QByteArray buffer;
        QString method;
        QString path;
        QMap<QString, QString> headers;
    };
    
    QMap<QTcpSocket*, ClientData> m_clients;
    
    void processHttpRequest(QTcpSocket* socket, const QByteArray& request);
    QJsonObject parseJsonBody(const QByteArray& body);
    void sendHttpResponse(QTcpSocket* socket, int statusCode, 
                         const QString& statusText, 
                         const QJsonObject& jsonData);
    void sendHttpResponse(QTcpSocket* socket, int statusCode, 
                         const QString& statusText, 
                         const QString& contentType, 
                         const QByteArray& body);
};

#endif