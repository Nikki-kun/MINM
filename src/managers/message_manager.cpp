#include "managers/message_manager.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QVariant>

namespace {
    QString threadConnectionName() {
        const auto tid = reinterpret_cast<quintptr>(QThread::currentThreadId());
        return QString("minm_mysql_conn_%1").arg(tid, 0, 16);
    }

    QSqlDatabase ensureDbOpen(const QString& host,
                               int port,
                               const QString& dbName,
                               const QString& user,
                               const QString& password) {
        const QString connName = threadConnectionName();

        QSqlDatabase db;
        if (QSqlDatabase::contains(connName)) {
            db = QSqlDatabase::database(connName);
        } else {
            db = QSqlDatabase::addDatabase("QMYSQL", connName);
        }

        db.setHostName(host);
        db.setPort(port);
        db.setDatabaseName(dbName);
        db.setUserName(user);
        db.setPassword(password);

        if (!db.open()) {
            qWarning() << "MySQL open failed:" << db.lastError().text();
            return QSqlDatabase();
        }

        return db;
    }

    chat_participant_role toRole(int value) {
        if (value < static_cast<int>(CHAT_ROLE_OWNER) || value > static_cast<int>(CHAT_ROLE_MEMBER)) {
            return CHAT_ROLE_MEMBER;
        }
        return static_cast<chat_participant_role>(value);
    }

    chat_participant_status toStatus(int value) {
        if (value < static_cast<int>(CHAT_MEMBER_ACTIVE) || value > static_cast<int>(CHAT_MEMBER_BANNED)) {
            return CHAT_MEMBER_ACTIVE;
        }
        return static_cast<chat_participant_status>(value);
    }
}

MessageManager::MessageManager(QVector<Contact>& contacts,
                              QVector<std::shared_ptr<Chat>>& chats,
                              QVector<std::shared_ptr<Message<std::string>>>& messages,
                              QObject* parent)
    : QObject(parent)
    , m_contacts(contacts)
    , m_chats(chats)
    , m_messages(messages)
{
    configureDbFromEnv();
    if (m_dbEnabled) {
        loadFromDb();
    }
}

void MessageManager::configureDbFromEnv() {
    const QByteArray enabledRaw = qgetenv("MINM_DB_ENABLED");
    if (enabledRaw.isEmpty()) {
        m_dbEnabled = false;
        return;
    }

    const QByteArray enabledLower = enabledRaw.toLower();
    m_dbEnabled = (enabledLower == "1" || enabledLower == "true" || enabledLower == "yes");
    if (!m_dbEnabled) return;

    m_dbHost = QString::fromUtf8(qgetenv("MINM_DB_HOST"));
    if (m_dbHost.isEmpty()) m_dbHost = "127.0.0.1";

    const QByteArray portRaw = qgetenv("MINM_DB_PORT");
    m_dbPort = portRaw.isEmpty() ? 3306 : portRaw.toInt();

    m_dbName = QString::fromUtf8(qgetenv("MINM_DB_NAME"));
    if (m_dbName.isEmpty()) m_dbName = "minm_test";

    m_dbUser = QString::fromUtf8(qgetenv("MINM_DB_USER"));
    if (m_dbUser.isEmpty()) m_dbUser = "minm";

    m_dbPassword = QString::fromUtf8(qgetenv("MINM_DB_PASSWORD"));
}

bool MessageManager::loadFromDb() {
    if (!m_dbEnabled) return false;

    auto db = ensureDbOpen(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword);
    if (!db.isValid() || !db.isOpen()) {
        m_dbEnabled = false;
        qWarning() << "MINM: MySQL connection failed, disabling DB persistence. Drivers:"
                   << QSqlDatabase::drivers();
        return false;
    }

    auto parseDt = [](const QVariant& v) -> QDateTime {
        QDateTime dt = v.toDateTime();
        if (dt.isValid()) return dt;
        const QString s = v.toString();
        if (s.isEmpty()) return QDateTime{};
        dt = QDateTime::fromString(s, "yyyy-MM-dd HH:mm:ss");
        if (dt.isValid()) return dt;
        return QDateTime{};
    };

    auto toTp = [](const QDateTime& dt) -> std::chrono::system_clock::time_point {
        if (!dt.isValid()) return std::chrono::system_clock::now();
        const qint64 ms = dt.toMSecsSinceEpoch();
        return std::chrono::system_clock::time_point{std::chrono::milliseconds(ms)};
    };

    db.transaction();

    m_contacts.clear();
    m_chats.clear();
    m_messages.clear();

    {
        QSqlQuery q(db);
        if (!q.exec("SELECT user_id, connected_user_id, connected_user_name, connected_at FROM user_interconnect WHERE type = 1 ORDER BY connected_at")) {
            qWarning() << "Contacts load failed:" << q.lastError().text();
            db.rollback();
            return false;
        }
        
        int loadedContacts = 0;
        while (q.next()) {
            user_id ownerId = q.value(0).toInt();
            user_id contactId = q.value(1).toInt();
            QString contactName = q.value(2).toString();
            QDateTime addedDate = parseDt(q.value(3));

            contact_id tempId = static_cast<contact_id>(loadedContacts + 1);
            m_contacts.append(Contact(tempId, ownerId, contactId, contactName, addedDate));
            loadedContacts++;
        }
        qWarning() << "MINM: loaded contacts rows:" << loadedContacts;
    }

    struct ChatInfo {
        chat_id id;
        chat_type type;
        std::chrono::system_clock::time_point created_at;
        QHash<user_id, ChatParticipantInfo> participants;
    };
    QHash<chat_id, ChatInfo> chatsMap;
    
    {
        QSqlQuery q(db);
        if (!q.exec(R"(
            SELECT 
                c.chat_id, 
                c.type, 
                c.chat_created_at,
                cp.user_id,
                cp.participant_role,
                cp.membership_status,
                cp.joined_at,
                cp.left_at,
                cp.banned_at
            FROM chats c
            LEFT JOIN chat_participants cp ON c.chat_id = cp.chat_id
            ORDER BY c.chat_id, cp.joined_at
        )")) {
            qWarning() << "Chats with participants load failed:" << q.lastError().text();
            db.rollback();
            return false;
        }

        chat_id lastChatId = -1;
        ChatInfo currentChat;
        
        while (q.next()) {
            chat_id chatId = q.value(0).toInt();
            
            if (chatId != lastChatId) {
                if (lastChatId != -1) {
                    chatsMap[lastChatId] = currentChat;
                }
                
                currentChat.id = chatId;
                currentChat.type = static_cast<chat_type>(q.value(1).toInt());
                currentChat.created_at = toTp(parseDt(q.value(2)));
                currentChat.participants.clear();
                lastChatId = chatId;
            }
            
            if (!q.value(3).isNull()) {
                ChatParticipantInfo info;
                info.role = toRole(q.value(4).toInt());
                info.status = toStatus(q.value(5).toInt());
                info.joined_at = toTp(parseDt(q.value(6)));
                
                if (!q.value(7).isNull()) {
                    info.left_at = toTp(parseDt(q.value(7)));
                }
                if (!q.value(8).isNull()) {
                    info.banned_at = toTp(parseDt(q.value(8)));
                }
                
                currentChat.participants[q.value(3).toInt()] = info;
            }
        }
        
        if (lastChatId != -1) {
            chatsMap[lastChatId] = currentChat;
        }
        
        qWarning() << "MINM: loaded chats with participants:" << chatsMap.size();
    }

    for (auto& chatInfo : chatsMap) {
        std::vector<user_id> activeParticipants;
        for (auto it = chatInfo.participants.begin(); it != chatInfo.participants.end(); ++it) {
            if (it.value().status == CHAT_MEMBER_ACTIVE) {
                activeParticipants.push_back(it.key());
            }
        }
        
        auto chat = std::make_shared<Chat>(
            chatInfo.id, 
            chatInfo.type, 
            activeParticipants, 
            std::vector<message_id>{}, 
            chatInfo.created_at
        );
        
        for (auto it = chatInfo.participants.begin(); it != chatInfo.participants.end(); ++it) {
            chat->setParticipantInfo(it.key(), it.value());
        }
        
        m_chats.append(chat);
    }

    {
        QSqlQuery q(db);
        if (!q.exec("SELECT message_id, sender_id, chat_id, content, type, status, message_created_at FROM messages ORDER BY message_created_at ASC")) {
            qWarning() << "Messages load failed:" << q.lastError().text();
            db.rollback();
            return false;
        }

        QHash<chat_id, QVector<message_id>> messagesByChat;
        
        int loadedMessages = 0;
        while (q.next()) {
            message_id id = q.value(0).toInt();
            user_id senderId = q.value(1).toInt();
            QVariant chatIdVar = q.value(2);
            chat_id receiverId = chatIdVar.isNull() ? -1 : chatIdVar.toInt();
            QString content = q.value(3).toString();
            
            message_type type = static_cast<message_type>(q.value(4).toInt());
            message_status status = static_cast<message_status>(q.value(5).toInt());
            QDateTime tsDt = parseDt(q.value(6));
            auto tsTp = toTp(tsDt);
            
            auto msg = std::make_shared<Message<std::string>>(
                id, senderId, receiverId, content.toStdString(), tsTp, type
            );
            msg->setStatus(status);
            m_messages.append(msg);
            
            if (receiverId != -1) {
                messagesByChat[receiverId].append(id);
            }
            loadedMessages++;
        }
        qWarning() << "MINM: loaded messages rows:" << loadedMessages;
        
        for (auto& chat : m_chats) {
            if (!chat) continue;
            auto& msgIds = messagesByChat[chat->id];
            for (message_id msgId : msgIds) {
                chat->addMessage(msgId);
            }
        }
    }

    db.commit();
    
    qWarning() << "MINM: after attach, chats:" << m_chats.size() 
               << "messages in memory:" << m_messages.size();
    return true;
}

QJsonObject MessageManager::handleRequest(const QString& method, const QString& path, const QJsonObject& data)
{
    QJsonObject response;
    
    if (method == "GET") {
        if (path == "/contacts") {
            response = handleGetContacts();
        } else if (path == "/chats") {
            response = handleGetChats();
        } else if (path == "/messages") {
            response = handleGetMessages();
        } else if (path == "/chat-participants") {
            response = handleGetChatParticipants(data);
        } else {
            response["error"] = "Unknown endpoint";
        }
    } else if (method == "POST") {
        if (path == "/contacts") {
            response = handlePostContacts(data);
        } else if (path == "/chats") {
            response = handlePostChats(data);
        } else if (path == "/messages") {
            response = handlePostMessages(data);
        } else if (path == "/chat-participants") {
            response = handlePostChatParticipants(data);
        } else {
            response["error"] = "Unknown endpoint";
        }
    } else {
        response["error"] = "Method not supported";
    }
    
    bool success = !response.contains("error");
    emit requestProcessed(method, path, success);
    
    return response;
}

bool MessageManager::addContact(const QJsonObject& data)
{
    if (!data.contains("ownerId") || !data.contains("contactId") || !data.contains("contactName")) {
        return false;
    }
    
    user_id ownerId = data["ownerId"].toInt();
    user_id contactId = data["contactId"].toInt();
    QString contactName = data["contactName"].toString();

    if (m_dbEnabled) {
        auto db = ensureDbOpen(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword);
        if (!db.isValid() || !db.isOpen()) {
            m_dbEnabled = false;
            qWarning() << "MINM: MySQL open failed, switching to in-memory.";
        } else {
            QSqlQuery q(db);
            // Используем INSERT IGNORE или ON DUPLICATE KEY UPDATE для избежания дублей
            q.prepare("INSERT INTO user_interconnect (user_id, connected_user_id, connected_user_name, type, connected_at) "
                      "VALUES (?, ?, ?, 1, NOW()) "
                      "ON DUPLICATE KEY UPDATE connected_user_name = VALUES(connected_user_name)");
            q.addBindValue(ownerId);
            q.addBindValue(contactId);
            q.addBindValue(contactName);

            if (!q.exec()) {
                qWarning() << "INSERT user_interconnect (contact) failed:" << q.lastError().text();
                return false;
            }

            if (!loadFromDb()) {
                if (m_dbEnabled) return false;
            } else {
                for (const auto& c : m_contacts) {
                    if (c.ownerId == ownerId && c.contactId == contactId) {
                        emit contactAdded(c);
                        return true;
                    }
                }
                emit contactAdded(Contact(0, ownerId, contactId, contactName));
                return true;
            }
        }
    }

    contact_id id = m_contacts.isEmpty() ? 1 : m_contacts.last().id + 1;
    
    Contact newContact(id, ownerId, contactId, contactName);
    m_contacts.append(newContact);
    
    emit contactAdded(newContact);
    return true;
}

bool MessageManager::removeContact(contact_id id)
{
    // Для удаления нужно знать owner_id и contact_id
    // Находим контакт по id
    const Contact* targetContact = nullptr;
    for (int i = 0; i < m_contacts.size(); i++) {
        if (m_contacts[i].id == id) {
            targetContact = &m_contacts[i];
            break;
        }
    }
    
    if (!targetContact) return false;
    
    if (m_dbEnabled) {
        auto db = ensureDbOpen(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword);
        if (!db.isValid() || !db.isOpen()) {
            m_dbEnabled = false;
            qWarning() << "MINM: MySQL open failed, switching to in-memory.";
        } else {
            QSqlQuery q(db);
            q.prepare("DELETE FROM user_interconnect WHERE user_id = ? AND connected_user_id = ? AND type = 1");
            q.addBindValue(targetContact->ownerId);
            q.addBindValue(targetContact->contactId);
            
            if (!q.exec()) {
                qWarning() << "DELETE from user_interconnect failed:" << q.lastError().text();
                return false;
            }
        }
    }
    
    for (int i = 0; i < m_contacts.size(); i++) {
        if (m_contacts[i].id == id) {
            m_contacts.remove(i);
            emit contactRemoved(id);
            return true;
        }
    }
    return false;
}

bool MessageManager::addChat(const QJsonObject& data)
{
    if (!data.contains("type") || !data.contains("participants")) {
        return false;
    }
    
    chat_type type = static_cast<chat_type>(data["type"].toInt());
    
    QJsonArray participantsArray = data["participants"].toArray();
    std::vector<user_id> participants;
    for (const auto& participant : participantsArray) {
        participants.push_back(participant.toInt());
    }

    if (m_dbEnabled) {
        auto db = ensureDbOpen(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword);
        if (!db.isValid() || !db.isOpen()) {
            m_dbEnabled = false;
            qWarning() << "MINM: MySQL open failed, switching to in-memory.";
        } else {
            QSqlQuery q(db);
            q.prepare("INSERT INTO chats (type) VALUES (?)");
            q.addBindValue(static_cast<int>(type));
            if (!q.exec()) {
                qWarning() << "INSERT chats failed:" << q.lastError().text();
                return false;
            }

            const int insertedId = q.lastInsertId().toInt();

            const user_id ownerId = participants.empty() ? -1 : participants.front();
            for (user_id u : participants) {
                QSqlQuery qp(db);
                qp.prepare("INSERT INTO chat_participants (chat_id, user_id, participant_role, membership_status) VALUES (?, ?, ?, ?)");
                qp.addBindValue(insertedId);
                qp.addBindValue(u);
                qp.addBindValue(static_cast<int>(u == ownerId ? CHAT_ROLE_OWNER : CHAT_ROLE_MEMBER));
                qp.addBindValue(static_cast<int>(CHAT_MEMBER_ACTIVE));
                if (!qp.exec()) {
                    qWarning() << "INSERT chat_participants failed:" << qp.lastError().text();
                    return false;
                }
            }

            if (!loadFromDb()) {
                if (m_dbEnabled) return false;
            } else {
                for (const auto& c : m_chats) {
                    if (c && c->id == insertedId) {
                        emit chatAdded(c);
                        return true;
                    }
                }
                return true;
            }
        }
    }

    chat_id id = m_chats.isEmpty() ? 1 : m_chats.last()->id + 1;
    
    auto newChat = std::make_shared<Chat>(id, type, participants);
    if (!participants.empty()) {
        for (const auto participant : participants) {
            ChatParticipantInfo info;
            info.role = (participant == participants.front()) ? CHAT_ROLE_OWNER : CHAT_ROLE_MEMBER;
            info.status = CHAT_MEMBER_ACTIVE;
            newChat->setParticipantInfo(participant, info);
        }
    }
    m_chats.append(newChat);
    
    emit chatAdded(newChat);
    return true;
}

bool MessageManager::removeChat(chat_id id)
{
    for (int i = 0; i < m_chats.size(); i++) {
        if (m_chats[i]->id == id) {
            m_chats.remove(i);
            emit chatRemoved(id);
            return true;
        }
    }
    return false;
}

bool MessageManager::addMessage(const QJsonObject& data)
{
    if (!data.contains("sender_id") || !data.contains("content")) {
        return false;
    }

    message_type msgType = static_cast<message_type>(data["type"].toInt(MESSAGE_NORMAL));
    chat_id receiver_id = data["receiver_id"].toInt(0);

    if (msgType == MESSAGE_NORMAL && !data.contains("receiver_id")) {
        return false;
    }

    if (m_dbEnabled) {
        auto db = ensureDbOpen(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword);
        if (!db.isValid() || !db.isOpen()) {
            m_dbEnabled = false;
            qWarning() << "MINM: MySQL open failed, switching to in-memory.";
        } else {

            user_id sender_id = data["sender_id"].toInt();
            const std::string content = data["content"].toString().toStdString();

            QSqlQuery q(db);
            q.prepare("INSERT INTO messages (sender_id, chat_id, content, status, type) VALUES (?, ?, ?, ?, ?)");
            q.addBindValue(sender_id);

            if (msgType == MESSAGE_BROADCAST) {
                q.addBindValue(QVariant());
            } else {
                q.addBindValue(receiver_id);
            }

            q.addBindValue(QString::fromStdString(content));
            q.addBindValue(static_cast<int>(SENT));
            q.addBindValue(static_cast<int>(msgType));

            if (!q.exec()) {
                qWarning() << "INSERT messages failed:" << q.lastError().text();
                return false;
            }

            if (!loadFromDb()) {
                if (m_dbEnabled) return false;
            } else {
                const int insertedId = q.lastInsertId().toInt();
                for (const auto& m : m_messages) {
                    if (m && m->id == insertedId) {
                        emit messageAdded(m);
                        return true;
                    }
                }

                const chat_id receiverForMsg = (msgType == MESSAGE_BROADCAST) ? -1 : receiver_id;
                auto dummy = std::make_shared<Message<std::string>>(0, sender_id, receiverForMsg, content, msgType);
                emit messageAdded(dummy);
                return true;
            }
        }
    }

    message_id id = m_messages.isEmpty() ? 1 : m_messages.last()->id + 1;
    user_id sender_id = data["sender_id"].toInt();
    std::string content = data["content"].toString().toStdString();

    if (msgType == MESSAGE_BROADCAST)
        receiver_id = -1;

    auto newMessage = std::make_shared<Message<std::string>>(id, sender_id, receiver_id, content, msgType);
    m_messages.append(newMessage);

    if (msgType == MESSAGE_BROADCAST) {
        for (auto& chat : m_chats) {
            chat->addMessage(id);
        }
    } else {
        for (auto& chat : m_chats) {
            if (chat->id == receiver_id) {
                chat->addMessage(id);
                break;
            }
        }
    }

    emit messageAdded(newMessage);
    return true;
}

bool MessageManager::removeMessage(message_id id)
{
    for (int i = 0; i < m_messages.size(); i++) {
        if (m_messages[i]->id == id) {
            bool isBroadcast = (m_messages[i]->type == MESSAGE_BROADCAST);
            chat_id chatId = m_messages[i]->receiver_id;
            m_messages.remove(i);
            if (isBroadcast) {
                for (auto& chat : m_chats) {
                    chat->removeMessage(id);
                }
            } else {
                for (auto& chat : m_chats) {
                    if (chat->id == chatId) {
                        chat->removeMessage(id);
                        break;
                    }
                }
            }
            emit messageRemoved(id);
            return true;
        }
    }
    return false;
}

QJsonObject MessageManager::handleGetContacts()
{
    if (m_dbEnabled) loadFromDb();
    QJsonObject response;
    QJsonArray contactsArray;
    
    for (const auto& contact : m_contacts) {
        QJsonObject contactObj;
        contactObj["id"] = static_cast<qint64>(contact.id);
        contactObj["ownerId"] = static_cast<qint64>(contact.ownerId);
        contactObj["contactId"] = static_cast<qint64>(contact.contactId);
        contactObj["contactName"] = contact.contactName;
        contactObj["addedDate"] = contact.addedDate.toString(Qt::ISODate);
        
        contactsArray.append(contactObj);
    }
    
    response["contacts"] = contactsArray;
    response["count"] = m_contacts.size();
    return response;
}

QJsonObject MessageManager::handlePostContacts(const QJsonObject& data)
{
    QJsonObject response;
    
    if (addContact(data)) {
        response["status"] = "success";
        response["message"] = "Contact added successfully";
    } else {
        response["status"] = "error";
        response["message"] = "Failed to add contact";
    }
    
    return response;
}

QJsonObject MessageManager::handleGetChats()
{
    if (m_dbEnabled) loadFromDb();
    QJsonObject response;
    QJsonArray chatsArray;
    
    for (const auto& chat : m_chats) {
        QJsonObject chatObj;
        chatObj["id"] = static_cast<qint64>(chat->id);
        chatObj["type"] = static_cast<int>(chat->type);
        
        QJsonArray participantsArray;
        auto participants = chat->getParticipants();
        for (const auto& participant : participants) {
            participantsArray.append(static_cast<qint64>(participant));
        }
        chatObj["participants"] = participantsArray;
        
        QJsonArray messagesArray;
        auto messages = chat->getMessages();
        for (const auto& message : messages) {
            messagesArray.append(static_cast<qint64>(message));
        }
        chatObj["messages"] = messagesArray;
        
        chatsArray.append(chatObj);
    }
    
    response["chats"] = chatsArray;
    response["count"] = m_chats.size();
    return response;
}

QJsonObject MessageManager::handlePostChats(const QJsonObject& data)
{
    QJsonObject response;
    
    if (addChat(data)) {
        response["status"] = "success";
        response["message"] = "Chat added successfully";
    } else {
        response["status"] = "error";
        response["message"] = "Failed to add chat";
    }
    
    return response;
}

QJsonObject MessageManager::handleGetMessages()
{
    if (m_dbEnabled) loadFromDb();
    QJsonObject response;
    QJsonArray messagesArray;
    
    for (const auto& message : m_messages) {
        QJsonObject messageObj;
        messageObj["id"] = static_cast<qint64>(message->id);
        messageObj["sender_id"] = static_cast<qint64>(message->sender_id);
        messageObj["receiver_id"] = static_cast<qint64>(message->receiver_id);
        messageObj["type"] = static_cast<int>(message->type);
        messageObj["content"] = QString::fromStdString(message->getContent());
        messageObj["status"] = static_cast<int>(message->getStatus());
        
        auto timestamp = message->timestamp;
        auto duration = timestamp.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        QDateTime qtime = QDateTime::fromMSecsSinceEpoch(milliseconds);
        messageObj["timestamp"] = qtime.toString(Qt::ISODateWithMs);
        
        messagesArray.append(messageObj);
    }
    
    response["messages"] = messagesArray;
    response["count"] = m_messages.size();
    return response;
}

QJsonObject MessageManager::handlePostMessages(const QJsonObject& data)
{
    QJsonObject response;
    
    if (addMessage(data)) {
        response["status"] = "success";
        response["message"] = "Message added successfully";
    } else {
        response["status"] = "error";
        response["message"] = "Failed to add message";
    }
    
    return response;
}

bool MessageManager::updateChatParticipant(const QJsonObject& data)
{
    if (!data.contains("chatId") || !data.contains("userId") || !data.contains("action")) {
        return false;
    }

    const chat_id chatId = data["chatId"].toInt();
    const user_id participantId = data["userId"].toInt();
    const QString action = data["action"].toString().trimmed().toLower();

    auto chatIt = std::find_if(m_chats.begin(), m_chats.end(), [chatId](const auto& c) { return c && c->id == chatId; });
    if (chatIt == m_chats.end()) return false;

    auto chat = *chatIt;
    ChatParticipantInfo info = chat->getParticipantInfo(participantId).value_or(ChatParticipantInfo{});

    if (action == "set_role") {
        if (!data.contains("role")) return false;
        info.role = toRole(data["role"].toInt());
    } else if (action == "ban") {
        info.status = CHAT_MEMBER_BANNED;
        info.banned_at = std::chrono::system_clock::now();
        info.left_at.reset();
    } else if (action == "leave") {
        info.status = CHAT_MEMBER_LEFT;
        info.left_at = std::chrono::system_clock::now();
        info.banned_at.reset();
    } else if (action == "activate") {
        info.status = CHAT_MEMBER_ACTIVE;
        info.left_at.reset();
        info.banned_at.reset();
    } else {
        return false;
    }

    if (m_dbEnabled) {
        auto db = ensureDbOpen(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword);
        if (!db.isValid() || !db.isOpen()) {
            m_dbEnabled = false;
            qWarning() << "MINM: MySQL open failed, switching to in-memory.";
        } else {
            QSqlQuery q(db);
            q.prepare(
                "INSERT INTO chat_participants (chat_id, user_id, participant_role, membership_status, joined_at, left_at, banned_at) "
                "VALUES (?, ?, ?, ?, NOW(), ?, ?) "
                "ON DUPLICATE KEY UPDATE "
                "participant_role=VALUES(participant_role), "
                "membership_status=VALUES(membership_status), "
                "left_at=VALUES(left_at), banned_at=VALUES(banned_at)");
            q.addBindValue(chatId);
            q.addBindValue(participantId);
            q.addBindValue(static_cast<int>(info.role));
            q.addBindValue(static_cast<int>(info.status));

            if (info.left_at.has_value()) {
                const auto leftMs = std::chrono::duration_cast<std::chrono::milliseconds>(info.left_at.value().time_since_epoch()).count();
                q.addBindValue(QDateTime::fromMSecsSinceEpoch(leftMs));
            } else {
                q.addBindValue(QVariant());
            }

            if (info.banned_at.has_value()) {
                const auto bannedMs = std::chrono::duration_cast<std::chrono::milliseconds>(info.banned_at.value().time_since_epoch()).count();
                q.addBindValue(QDateTime::fromMSecsSinceEpoch(bannedMs));
            } else {
                q.addBindValue(QVariant());
            }

            if (!q.exec()) {
                qWarning() << "UPSERT chat_participants failed:" << q.lastError().text();
                return false;
            }
        }
    }

    chat->setParticipantInfo(participantId, info);
    return true;
}

QJsonObject MessageManager::handleGetChatParticipants(const QJsonObject& data)
{
    if (m_dbEnabled) loadFromDb();
    QJsonObject response;

    if (!data.contains("chatId")) {
        response["error"] = "chatId is required";
        return response;
    }

    const chat_id chatId = data["chatId"].toInt();
    auto chatIt = std::find_if(m_chats.begin(), m_chats.end(), [chatId](const auto& c) { return c && c->id == chatId; });
    if (chatIt == m_chats.end()) {
        response["error"] = "Chat not found";
        return response;
    }

    QJsonArray participantsArray;
    const auto meta = (*chatIt)->getAllParticipantInfo();
    for (const auto& [uid, info] : meta) {
        QJsonObject participantObj;
        participantObj["userId"] = static_cast<qint64>(uid);
        participantObj["role"] = static_cast<int>(info.role);
        participantObj["status"] = static_cast<int>(info.status);
        const auto joinedMs = std::chrono::duration_cast<std::chrono::milliseconds>(info.joined_at.time_since_epoch()).count();
        participantObj["joinedAt"] = QDateTime::fromMSecsSinceEpoch(joinedMs).toString(Qt::ISODateWithMs);
        if (info.left_at.has_value()) {
            const auto leftMs = std::chrono::duration_cast<std::chrono::milliseconds>(info.left_at.value().time_since_epoch()).count();
            participantObj["leftAt"] = QDateTime::fromMSecsSinceEpoch(leftMs).toString(Qt::ISODateWithMs);
        }
        if (info.banned_at.has_value()) {
            const auto bannedMs = std::chrono::duration_cast<std::chrono::milliseconds>(info.banned_at.value().time_since_epoch()).count();
            participantObj["bannedAt"] = QDateTime::fromMSecsSinceEpoch(bannedMs).toString(Qt::ISODateWithMs);
        }
        participantsArray.append(participantObj);
    }

    response["chatId"] = static_cast<qint64>(chatId);
    response["participants"] = participantsArray;
    response["count"] = participantsArray.size();
    return response;
}

QJsonObject MessageManager::handlePostChatParticipants(const QJsonObject& data)
{
    QJsonObject response;

    if (updateChatParticipant(data)) {
        response["status"] = "success";
        response["message"] = "Chat participant updated";
    } else {
        response["status"] = "error";
        response["message"] = "Failed to update chat participant";
    }

    return response;
}

QVector<Contact> MessageManager::getContacts() const
{
    return m_contacts;
}

QVector<std::shared_ptr<Chat>> MessageManager::getChats() const
{
    return m_chats;
}

QVector<std::shared_ptr<Message<std::string>>> MessageManager::getMessages() const
{
    return m_messages;
}