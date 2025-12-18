#include "http_server.h"
#include <QDebug>
#include <QJsonParseError>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include <QMutexLocker>

class RequestProcessor : public QObject
{
    Q_OBJECT
    
public:
    RequestProcessor(MessageManager& manager, QObject* parent = nullptr) 
        : QObject(parent), m_manager(manager) 
    {
        qDebug() << "RequestProcessor created in thread:" << QThread::currentThread();
    }
    
    ~RequestProcessor() {
        qDebug() << "RequestProcessor destroyed";
    }
    
public slots:
    void processRequest(qintptr socketDescriptor, const QByteArray& requestData)
    {
        qDebug() << "[" << QThread::currentThread()->objectName() 
                 << "] Processing request for socket:" << socketDescriptor;
        
        // Парсим запрос
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
        
        qDebug() << "[" << QThread::currentThread()->objectName() 
                 << "] " << method << path;
        
        QJsonObject jsonData;
        int bodyIndex = requestStr.indexOf("\r\n\r\n");
        if (bodyIndex != -1) {
            QByteArray body = requestStr.mid(bodyIndex + 4).toUtf8();
            if (!body.isEmpty()) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
                if (parseError.error == QJsonParseError::NoError) {
                    jsonData = doc.object();
                } else {
                    qWarning() << "JSON parse error:" << parseError.errorString();
                }
            }
        }
        
        QJsonObject response;
        {
            static QMutex logMutex;
            QMutexLocker locker(&logMutex);
            qDebug() << "[" << QThread::currentThread()->objectName() 
                     << "] Calling MessageManager::handleRequest()";
        }
        
        response = m_manager.handleRequest(method, path, jsonData);
        
        {
            static QMutex logMutex;
            QMutexLocker locker(&logMutex);
            qDebug() << "[" << QThread::currentThread()->objectName() 
                     << "] Request processed, sending response";
        }
        
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
        } else {
            qWarning() << "Failed to set socket descriptor for response:" << socket->errorString();
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
    // Регистрируем qintptr как метатип
    qRegisterMetaType<qintptr>("qintptr");
    
    // Создаём обработчик запросов
    m_requestProcessor = new RequestProcessor(manager);
    
    // Создаём отдельный поток для обработки запросов
    m_processorThread = new QThread();
    m_processorThread->setObjectName("RequestProcessorThread");
    
    // Перемещаем обработчик в поток
    m_requestProcessor->moveToThread(m_processorThread);
    
    // Регистрируем qintptr для сигналов/слотов
    qRegisterMetaType<qintptr>();
    
    // Подключаем сигнал завершения обработки
    connect(m_requestProcessor, &RequestProcessor::requestProcessed,
            this, [this](qintptr socketDescriptor) {
                // Удаляем сокет из мапы после обработки
                if (m_sockets.contains(socketDescriptor)) {
                    QTcpSocket* socket = m_sockets.take(socketDescriptor);
                    if (socket) {
                        socket->close();
                        socket->deleteLater();
                    }
                    m_clients.remove(socketDescriptor);
                }
            });
    
    // Запускаем поток
    m_processorThread->start();
    
    qDebug() << "HTTP Server created. Main thread:" << QThread::currentThread();
    qDebug() << "Request processor thread:" << m_processorThread->objectName();
    
    // Подключаем сигналы сервера
    connect(&m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

HttpServer::~HttpServer()
{
    qDebug() << "Shutting down HTTP server...";
    
    if (m_processorThread) {
        m_processorThread->quit();
        m_processorThread->wait();
        delete m_processorThread;
    }
    
    if (m_requestProcessor) {
        delete m_requestProcessor;
    }
    
    // Закрываем все сокеты
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
    if (!m_server.listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start HTTP server on port" << port << ":" << m_server.errorString();
        return false;
    }
    
    qInfo() << "===============================================";
    qInfo() << "HTTP server started on port" << port;
    qInfo() << "Main thread:" << QThread::currentThread();
    qInfo() << "Processor thread:" << m_processorThread->objectName();
    qInfo() << "===============================================";
    qInfo() << "Available endpoints:";
    qInfo() << "  GET  /contacts";
    qInfo() << "  POST /contacts";
    qInfo() << "  GET  /chats";
    qInfo() << "  POST /chats";
    qInfo() << "  GET  /messages";
    qInfo() << "  POST /messages";
    qInfo() << "===============================================";
    
    return true;
}

void HttpServer::onNewConnection()
{
    QTcpSocket* socket = m_server.nextPendingConnection();
    if (!socket) return;
    
    qintptr socketDescriptor = socket->socketDescriptor();
    
    qDebug() << "[Main Thread] New connection from" 
             << socket->peerAddress().toString() 
             << "socket descriptor:" << socketDescriptor;
    
    // Сохраняем сокет и его дескриптор
    m_sockets[socketDescriptor] = socket;
    m_clients[socketDescriptor] = ClientData(socketDescriptor);
    
    // Подключаем сигналы
    connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, [this, socketDescriptor]() {
        qDebug() << "[Main Thread] Socket disconnected:" << socketDescriptor;
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
    
    // Читаем данные
    QByteArray data = socket->readAll();
    
    if (data.isEmpty()) {
        qWarning() << "Empty data from socket:" << socketDescriptor;
        return;
    }
    
    // Добавляем в буфер
    if (!m_clients.contains(socketDescriptor)) {
        m_clients[socketDescriptor] = ClientData(socketDescriptor);
    }
    
    ClientData& clientData = m_clients[socketDescriptor];
    clientData.buffer.append(data);
    
    // Проверяем завершённость HTTP запроса
    if (clientData.buffer.contains("\r\n\r\n")) {
        qDebug() << "[Main Thread] Full request received from socket:" 
                 << socketDescriptor << "size:" << clientData.buffer.size();
        
        // Получаем данные запроса
        QByteArray requestData = clientData.buffer;
        clientData.buffer.clear();
        
        // !!! ПЕРЕДАЁМ ОБРАБОТКУ В ОТДЕЛЬНЫЙ ПОТОК !!!
        // Теперь мьютексы в Chat, User, Message будут реально использоваться
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
    // Этот метод больше не используется напрямую
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
    // Этот метод больше не используется напрямую
    Q_UNUSED(socketDescriptor);
    Q_UNUSED(statusCode);
    Q_UNUSED(statusText);
    Q_UNUSED(jsonData);
}

#include "http_server.moc"