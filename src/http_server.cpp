#include "http_server.h"
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class RequestProcessor : public QObject
{
    Q_OBJECT
    
public:
    RequestProcessor(MessageManager& manager, QObject* parent = nullptr)
        : QObject(parent), m_manager(manager) {}
    
public slots:
    void processRequest(qintptr socketDescriptor, const QByteArray& requestData)
    {
        
        QString requestStr = QString::fromUtf8(requestData);
        QStringList lines = requestStr.split("\r\n");
        
        if (lines.isEmpty()) {
            sendResponse(socketDescriptor, 400, "Bad Request", "text/plain", "Invalid request");
            emit requestProcessed(socketDescriptor);
            return;
        }
        
        QStringList firstLine = lines[0].split(" ");
        if (firstLine.size() < 2) {
            sendResponse(socketDescriptor, 400, "Bad Request", "text/plain", "Invalid request line");
            emit requestProcessed(socketDescriptor);
            return;
        }
        
        QString method = firstLine[0].toUpper();
        QString path = firstLine[1];
        
        
        QJsonObject jsonData;
        int bodyIndex = requestStr.indexOf("\r\n\r\n");
        if (bodyIndex != -1) {
            QByteArray body = requestStr.mid(bodyIndex + 4).toUtf8();
            if (!body.isEmpty()) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
                if (parseError.error == QJsonParseError::NoError) {
                    jsonData = doc.object();
                }
            }
        }
        
        QJsonObject response = m_manager.handleRequest(method, path, jsonData);
        
        QJsonDocument doc(response);
        sendResponse(socketDescriptor, 200, "OK", "application/json", doc.toJson(QJsonDocument::Indented));
        
        emit requestProcessed(socketDescriptor);
    }
    
signals:
    void requestProcessed(qintptr socketDescriptor);
    
private:
    void sendResponse(qintptr socketDescriptor, int statusCode, const QString& statusText,
                     const QString& contentType, const QByteArray& body)
    {
        QTcpSocket* socket = new QTcpSocket();
        
        if (socket->setSocketDescriptor(socketDescriptor)) {
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
            socket->waitForBytesWritten(3000);
            socket->close();
        }
        
        socket->deleteLater();
    }
    
private:
    MessageManager& m_manager;
};

HttpServer::HttpServer(MessageManager& manager, QObject* parent)
    : QObject(parent)
    , m_requestProcessor(nullptr)
    , m_processorThread(nullptr)
    , m_manager(manager)
{
    qRegisterMetaType<qintptr>("qintptr");
    
    m_requestProcessor = new RequestProcessor(manager);
    
    m_processorThread = new QThread();
    m_processorThread->setObjectName("RequestProcessorThread");
    
    m_requestProcessor->moveToThread(m_processorThread);
    
    qRegisterMetaType<qintptr>();
    
    connect(m_requestProcessor, &RequestProcessor::requestProcessed,
            this, [this](qintptr socketDescriptor) {
                if (m_sockets.contains(socketDescriptor)) {
                    QTcpSocket* socket = m_sockets.take(socketDescriptor);
                    if (socket) {
                        socket->close();
                        socket->deleteLater();
                    }
                    m_clients.remove(socketDescriptor);
                }
            });
    
    m_processorThread->start();
    connect(&m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

HttpServer::~HttpServer()
{
    if (m_processorThread) {
        m_processorThread->quit();
        m_processorThread->wait();
        delete m_processorThread;
    }
    
    if (m_requestProcessor) {
        delete m_requestProcessor;
    }
    
    for (QTcpSocket* socket : m_sockets.values()) {
        if (socket) {
            socket->close();
            socket->deleteLater();
        }
    }
    m_sockets.clear();
    m_clients.clear();
}

bool HttpServer::start(quint16 port)
{
    return m_server.listen(QHostAddress::Any, port);
}

void HttpServer::onNewConnection()
{
    QTcpSocket* socket = m_server.nextPendingConnection();
    if (!socket) return;
    
    qintptr socketDescriptor = socket->socketDescriptor();
    m_sockets[socketDescriptor] = socket;
    m_clients[socketDescriptor] = ClientData(socketDescriptor);
    
    connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, [this, socketDescriptor]() {
        if (m_sockets.contains(socketDescriptor)) {
            QTcpSocket* socket = m_sockets.take(socketDescriptor);
            if (socket) {
                socket->deleteLater();
            }
            m_clients.remove(socketDescriptor);
        }
    });
}

void HttpServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    
    qintptr socketDescriptor = socket->socketDescriptor();
    
    QByteArray data = socket->readAll();
    
    if (data.isEmpty()) return;
    
    if (!m_clients.contains(socketDescriptor)) {
        m_clients[socketDescriptor] = ClientData(socketDescriptor);
    }
    
    ClientData& clientData = m_clients[socketDescriptor];
    clientData.buffer.append(data);
    
    if (clientData.buffer.contains("\r\n\r\n")) {
        QByteArray requestData = clientData.buffer;
        clientData.buffer.clear();
        
        QMetaObject::invokeMethod(m_requestProcessor, "processRequest",
                                  Qt::QueuedConnection,
                                  Q_ARG(qintptr, socketDescriptor),
                                  Q_ARG(QByteArray, requestData));
    }
}

void HttpServer::sendHttpResponse(qintptr socketDescriptor, int statusCode, 
                                 const QString& statusText, 
                                 const QString& contentType, 
                                 const QByteArray& body)
{
    Q_UNUSED(socketDescriptor);
    Q_UNUSED(statusCode);
    Q_UNUSED(statusText);
    Q_UNUSED(contentType);
    Q_UNUSED(body);
}

void HttpServer::sendHttpResponse(qintptr socketDescriptor, int statusCode, 
                                 const QString& statusText, 
                                 const QJsonObject& jsonData)
{
    Q_UNUSED(socketDescriptor);
    Q_UNUSED(statusCode);
    Q_UNUSED(statusText);
    Q_UNUSED(jsonData);
}

#include "http_server.moc"