#include "http_server.h"
#include <QDebug>
#include <QJsonParseError>
#include <QDateTime>

HttpServer::HttpServer(MessageManager& manager, QObject* parent)
    : QObject(parent)
    , m_manager(manager)
{
    connect(&m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

bool HttpServer::start(quint16 port)
{
    if (!m_server.listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start HTTP server on port" << port << ":" << m_server.errorString();
        return false;
    }
    
    qInfo() << "HTTP server started on port" << port;
    qInfo() << "Available endpoints:";
    qInfo() << "  GET  /contacts";
    qInfo() << "  POST /contacts";
    qInfo() << "  GET  /chats";
    qInfo() << "  POST /chats";
    qInfo() << "  GET  /messages";
    qInfo() << "  POST /messages";
    
    return true;
}

void HttpServer::onNewConnection()
{
    QTcpSocket* socket = m_server.nextPendingConnection();
    if (socket) {
        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &HttpServer::onDisconnected);
        
        m_clients[socket] = ClientData();
        
        qDebug() << "New connection from" << socket->peerAddress().toString();
    }
}

void HttpServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    
    ClientData& clientData = m_clients[socket];
    clientData.buffer.append(socket->readAll());
    
    if (clientData.buffer.contains("\r\n\r\n")) {
        processHttpRequest(socket, clientData.buffer);
        clientData.buffer.clear();
    }
}

void HttpServer::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        m_clients.remove(socket);
        socket->deleteLater();
        qDebug() << "Client disconnected";
    }
}

void HttpServer::processHttpRequest(QTcpSocket* socket, const QByteArray& request)
{
    QString requestStr = QString::fromUtf8(request);
    QStringList lines = requestStr.split("\r\n");
    
    if (lines.isEmpty()) {
        sendHttpResponse(socket, 400, "Bad Request", "text/plain", "Invalid request");
        return;
    }
    
    QStringList firstLine = lines[0].split(" ");
    if (firstLine.size() < 2) {
        sendHttpResponse(socket, 400, "Bad Request", "text/plain", "Invalid request line");
        return;
    }
    
    QString method = firstLine[0].toUpper();
    QString path = firstLine[1];
    
    qDebug() << "Processing" << method << "request to" << path;
    
    QMap<QString, QString> headers;
    int i;
    for (i = 1; i < lines.size(); i++) {
        if (lines[i].isEmpty()) break;
        QStringList header = lines[i].split(": ");
        if (header.size() >= 2) {
            headers[header[0].toLower()] = header[1];
        }
    }
    
    QByteArray body;
    if (i + 1 < lines.size()) {
        body = lines[i + 1].toUtf8();
    }
    
    QJsonObject jsonData;
    
    if (method == "POST" || method == "PUT") {
        if (headers["content-type"].contains("application/json")) {
            jsonData = parseJsonBody(body);
        }
    }
    
    QJsonObject response = m_manager.handleRequest(method, path, jsonData);
    
    sendHttpResponse(socket, 200, "OK", response);
}

QJsonObject HttpServer::parseJsonBody(const QByteArray& body)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

void HttpServer::sendHttpResponse(QTcpSocket* socket, int statusCode, 
                                 const QString& statusText, 
                                 const QJsonObject& jsonData)
{
    QJsonDocument doc(jsonData);
    QByteArray json = doc.toJson(QJsonDocument::Indented);
    
    sendHttpResponse(socket, statusCode, statusText, "application/json", json);
}

void HttpServer::sendHttpResponse(QTcpSocket* socket, int statusCode, 
                                 const QString& statusText, 
                                 const QString& contentType, 
                                 const QByteArray& body)
{
    QString response = QString("HTTP/1.1 %1 %2\r\n"
                               "Content-Type: %3\r\n"
                               "Content-Length: %4\r\n"
                               "Connection: close\r\n"
                               "Access-Control-Allow-Origin: *\r\n"
                               "Server: MessengerServer/1.0\r\n"
                               "\r\n")
                       .arg(statusCode)
                       .arg(statusText)
                       .arg(contentType)
                       .arg(body.size());
    
    socket->write(response.toUtf8());
    socket->write(body);
    socket->flush();
    socket->close();
}