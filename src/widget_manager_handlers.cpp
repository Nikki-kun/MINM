#include "widget_manager.h"
#include "core/types.h"
#include <QDateTime>
#include <algorithm>

QString WidgetManager::roleToLabel(chat_participant_role role) const
{
    switch (role) {
    case CHAT_ROLE_OWNER: return "Owner";
    case CHAT_ROLE_ADMIN: return "Admin";
    case CHAT_ROLE_MEMBER:
    default: return "Member";
    }
}

QString WidgetManager::statusToLabel(chat_participant_status status) const
{
    switch (status) {
    case CHAT_MEMBER_ACTIVE: return "Active";
    case CHAT_MEMBER_LEFT: return "Left";
    case CHAT_MEMBER_BANNED: return "Banned";
    default: return "Unknown";
    }
}

void WidgetManager::updateContacts()
{
    m_contactsTree->clear();
    for (const Contact& contact : m_contacts) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_contactsTree);
        item->setText(0, QString("👤 %1").arg(contact.contactName));
        item->setData(0, Qt::UserRole, QVariant::fromValue(contact.id));
        item->setData(0, Qt::UserRole + 1, QVariant::fromValue(contact.contactId));
        item->setToolTip(0, QString("<b>Контакт:</b> %1<br><b>ID контакта:</b> %2<br>"
            "<b>Владелец:</b> %3<br><b>Добавлен:</b> %4")
            .arg(contact.contactName).arg(contact.contactId)
            .arg(contact.ownerId).arg(contact.addedDate.toString("dd.MM.yyyy HH:mm")));
        item->setForeground(0, QBrush(QColor("#d4d4d4")));
    }
    m_contactsTree->resizeColumnToContents(0);
}

void WidgetManager::updateChats()
{
    m_chatsTree->clear();
    for (const auto& chat : m_chats) {
        if (m_selectedContactUserId >= 0) {
            auto participants = chat->getParticipants();
            bool hasUser = std::find(participants.begin(), participants.end(), CURRENT_USER_ID) != participants.end();
            bool hasContact = std::find(participants.begin(), participants.end(), m_selectedContactUserId) != participants.end();
            if (!hasUser || !hasContact)
                continue;
        }
        QTreeWidgetItem *item = new QTreeWidgetItem(m_chatsTree);
        item->setText(0, QString("#%1").arg(chat->id));
        QString typeStr = (chat->type == chat_type::PRIVATE) ? "🔒 Приватный" : "👥 Групповой";
        item->setText(1, typeStr);
        item->setText(2, QString("👤 %1").arg(chat->getParticipants().size()));
        item->setText(3, QString("💬 %1").arg(chat->getMessages().size()));
        auto timePoint = chat->created_date;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timePoint.time_since_epoch()).count();
        item->setText(4, QDateTime::fromSecsSinceEpoch(seconds).toString("dd.MM.yyyy HH:mm"));
        item->setData(0, Qt::UserRole, QVariant::fromValue(chat->id));
        item->setForeground(0, QBrush(QColor("#4ec9b0")));
        item->setForeground(1, QBrush(QColor(chat->type == chat_type::PRIVATE ? "#ce9178" : "#4ec9b0")));
        item->setForeground(2, QBrush(QColor("#dcdcaa")));
        item->setForeground(3, QBrush(QColor("#dcdcaa")));
        item->setForeground(4, QBrush(QColor("#858585")));
    }
    for (int i = 0; i < 5; i++)
        m_chatsTree->resizeColumnToContents(i);
}

void WidgetManager::updateMessages()
{
    updateMessagesForChat(m_selectedChatId);
}

void WidgetManager::updateSendButtonState()
{
    m_sendButton->setEnabled(m_selectedChatId >= 0);
    m_messageInput->setEnabled(m_selectedChatId >= 0);
    m_messageInput->setPlaceholderText(m_selectedChatId >= 0 ? "Введите сообщение..." : "Выберите чат для отправки сообщения");
}

void WidgetManager::updateMessagesForChat(chat_id chatId)
{
    m_messagesTable->clearContents();
    m_messagesTable->setRowCount(0);
    if (chatId < 0) {
        m_messagesLabel->setText("📨 Сообщения — выберите чат");
        return;
    }
    m_messagesLabel->setText("📨 Сообщения");
    auto chatIt = std::find_if(m_chats.begin(), m_chats.end(), [chatId](const auto& c) { return c->id == chatId; });
    if (chatIt == m_chats.end()) return;
    QVector<std::shared_ptr<Message<std::string>>> chatMessages;
    for (message_id msgId : (*chatIt)->getMessages()) {
        auto msgIt = std::find_if(m_messages.begin(), m_messages.end(), [msgId](const auto& m) { return m->id == msgId; });
        if (msgIt != m_messages.end())
            chatMessages.append(*msgIt);
    }
    m_messagesTable->setRowCount(chatMessages.size());
    for (int i = 0; i < chatMessages.size(); i++) {
        const auto& message = chatMessages[i];
        QTableWidgetItem *idItem = new QTableWidgetItem(QString("#%1").arg(message->id));
        idItem->setData(Qt::UserRole, QVariant::fromValue(message->id));
        idItem->setForeground(QBrush(QColor("#4ec9b0")));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        QTableWidgetItem *senderItem = new QTableWidgetItem(QString("👤 %1").arg(message->sender_id));
        senderItem->setForeground(QBrush(QColor("#ce9178")));
        senderItem->setFlags(senderItem->flags() & ~Qt::ItemIsEditable);
        QString receiverText = (message->type == MESSAGE_BROADCAST) ? QString("📢 Все чаты") : QString("📨 %1").arg(message->receiver_id);
        QTableWidgetItem *receiverItem = new QTableWidgetItem(receiverText);
        receiverItem->setForeground(QBrush(QColor("#569cd6")));
        receiverItem->setFlags(receiverItem->flags() & ~Qt::ItemIsEditable);
        QString content = QString::fromStdString(message->getContent());
        if (content.length() > 50) content = content.left(47) + "...";
        QTableWidgetItem *contentItem = new QTableWidgetItem(content);
        contentItem->setForeground(QBrush(QColor("#d4d4d4")));
        contentItem->setToolTip(QString::fromStdString(message->getContent()));
        contentItem->setFlags(contentItem->flags() & ~Qt::ItemIsEditable);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(message->timestamp.time_since_epoch()).count();
        QTableWidgetItem *timeItem = new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(ms).toString("dd.MM.yyyy HH:mm:ss"));
        timeItem->setForeground(QBrush(QColor("#858585")));
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
        m_messagesTable->setItem(i, 0, idItem);
        m_messagesTable->setItem(i, 1, senderItem);
        m_messagesTable->setItem(i, 2, receiverItem);
        m_messagesTable->setItem(i, 3, contentItem);
        m_messagesTable->setItem(i, 4, timeItem);
    }
    m_messagesTable->resizeColumnsToContents();
}

void WidgetManager::updateAll()
{
    updateContacts();
    updateChats();
    updateMessages();
    updateSendButtonState();
}

void WidgetManager::onContactSelected(int index)
{
    if (index < 0 || index >= m_contacts.size()) return;
    const Contact& contact = m_contacts[index];
    m_selectedContactUserId = contact.contactId;
    m_selectedChatId = -1;
    m_chatsTree->clearSelection();
    emit contactSelected(contact.id);
    updateChats();
    updateMessagesForChat(-1);
    updateSendButtonState();
    m_messageDetails->setHtml(
        QString("<div style='font-family: Consolas, Monaco, monospace; line-height: 1.6;'>"
            "<h3 style='color: #4ec9b0; margin-top: 0;'>👤 Детали контакта</h3>"
            "<p><b style='color: #ce9178;'>ID:</b> <span style='color: #4ec9b0;'>%1</span></p>"
            "<p><b style='color: #ce9178;'>Владелец:</b> <span style='color: #dcdcaa;'>%2</span></p>"
            "<p><b style='color: #ce9178;'>Контакт ID:</b> <span style='color: #4ec9b0;'>%3</span></p>"
            "<p><b style='color: #ce9178;'>Имя:</b> <span style='color: #d4d4d4;'>%4</span></p>"
            "<p><b style='color: #ce9178;'>Добавлен:</b> <span style='color: #858585;'>%5</span></p></div>")
        .arg(contact.id).arg(contact.ownerId).arg(contact.contactId)
        .arg(contact.contactName).arg(contact.addedDate.toString("dd.MM.yyyy HH:mm:ss")));
}

void WidgetManager::onChatSelected(chat_id chatId)
{
    auto it = std::find_if(m_chats.begin(), m_chats.end(), [chatId](const auto& c) { return c->id == chatId; });
    if (it == m_chats.end()) return;
    const auto& chat = *it;
    m_selectedChatId = chat->id;
    emit chatSelected(chat->id);
    updateMessagesForChat(chat->id);
    updateSendButtonState();
    auto participants = chat->getParticipants();
    auto participantsMeta = chat->getAllParticipantInfo();
    auto messages = chat->getMessages();
    QString participantsStr, messagesStr, participantsMetaStr;
    for (size_t i = 0; i < participants.size(); i++) {
        participantsStr += QString::number(participants[i]);
        if (i < participants.size() - 1) participantsStr += ", ";
    }
    for (size_t i = 0; i < messages.size(); i++) {
        messagesStr += QString::number(messages[i]);
        if (i < messages.size() - 1) messagesStr += ", ";
    }
    for (const auto& [uid, info] : participantsMeta) {
        QString role = roleToLabel(info.role);
        QString status = statusToLabel(info.status);
        participantsMetaStr += QString("%1 (%2, %3)").arg(uid).arg(role).arg(status);
        if (info.left_at.has_value()) {
            const auto leftMs = std::chrono::duration_cast<std::chrono::milliseconds>(info.left_at.value().time_since_epoch()).count();
            participantsMetaStr += QString(", left: %1").arg(QDateTime::fromMSecsSinceEpoch(leftMs).toString("dd.MM.yyyy HH:mm:ss"));
        }
        if (info.banned_at.has_value()) {
            const auto bannedMs = std::chrono::duration_cast<std::chrono::milliseconds>(info.banned_at.value().time_since_epoch()).count();
            participantsMetaStr += QString(", banned: %1").arg(QDateTime::fromMSecsSinceEpoch(bannedMs).toString("dd.MM.yyyy HH:mm:ss"));
        }
        participantsMetaStr += "<br>";
    }
    QString typeIcon = (chat->type == chat_type::PRIVATE) ? "🔒" : "👥";
    QString typeText = (chat->type == chat_type::PRIVATE) ? "Приватный" : "Групповой";
    m_messageDetails->setHtml(
        QString("<div style='font-family: Consolas, Monaco, monospace; line-height: 1.6;'>"
            "<h3 style='color: #4ec9b0; margin-top: 0;'>💬 Детали чата</h3>"
            "<p><b style='color: #ce9178;'>ID:</b> <span style='color: #4ec9b0;'>#%1</span></p>"
            "<p><b style='color: #ce9178;'>Тип:</b> <span style='color: %7;'>%8 %2</span></p>"
            "<p><b style='color: #ce9178;'>Участников:</b> <span style='color: #dcdcaa;'>👤 %3</span></p>"
            "<p><b style='color: #ce9178;'>Сообщений:</b> <span style='color: #dcdcaa;'>💬 %4</span></p>"
            "<p><b style='color: #ce9178;'>Участники:</b> <span style='color: #d4d4d4;'>%5</span></p>"
            "<p><b style='color: #ce9178;'>Роли/статусы:</b><br><span style='color: #d4d4d4;'>%9</span></p>"
            "<p><b style='color: #ce9178;'>Сообщения ID:</b> <span style='color: #858585;'>%6</span></p></div>")
        .arg(chat->id).arg(typeText).arg(participants.size()).arg(messages.size())
        .arg(participantsStr).arg(messagesStr)
        .arg(chat->type == chat_type::PRIVATE ? "#ce9178" : "#4ec9b0").arg(typeIcon)
        .arg(participantsMetaStr.isEmpty() ? "-" : participantsMetaStr));
}

void WidgetManager::onMessageSelected(int row)
{
    if (row < 0) return;
    QTableWidgetItem *idItem = m_messagesTable->item(row, 0);
    if (!idItem) return;
    message_id msgId = idItem->data(Qt::UserRole).value<message_id>();
    auto it = std::find_if(m_messages.begin(), m_messages.end(), [msgId](const auto& m) { return m->id == msgId; });
    if (it == m_messages.end()) return;
    const auto& message = *it;
    emit messageSelected(message->id);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(message->timestamp.time_since_epoch()).count();
    QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(ms);
    QString statusText, statusColor, statusIcon;
    if (message->getStatus() == SENT) { statusText = "Отправлено"; statusColor = "#858585"; statusIcon = "📤"; }
    else if (message->getStatus() == DELIVERED) { statusText = "Доставлено"; statusColor = "#dcdcaa"; statusIcon = "✓"; }
    else { statusText = "Прочитано"; statusColor = "#4ec9b0"; statusIcon = "✓✓"; }
    QString receiverStr = (message->type == MESSAGE_BROADCAST) ? "📢 Все чаты (рассылка)" : QString("📨 %1").arg(message->receiver_id);
    m_messageDetails->setHtml(
        QString("<div style='font-family: Consolas, Monaco, monospace; line-height: 1.6;'>"
            "<h3 style='color: #4ec9b0; margin-top: 0;'>📨 Детали сообщения</h3>"
            "<p><b style='color: #ce9178;'>ID:</b> <span style='color: #4ec9b0;'>#%1</span></p>"
            "<p><b style='color: #ce9178;'>Тип:</b> <span style='color: #dcdcaa;'>%9</span></p>"
            "<p><b style='color: #ce9178;'>Отправитель:</b> <span style='color: #ce9178;'>👤 %2</span></p>"
            "<p><b style='color: #ce9178;'>Получатель:</b> <span style='color: #569cd6;'>%3</span></p>"
            "<p><b style='color: #ce9178;'>Контент:</b></p>"
            "<div style='background-color: #1e1e1e; padding: 8px; border-left: 3px solid #0078d4; margin: 8px 0; border-radius: 4px;'>"
            "<span style='color: #d4d4d4;'>%4</span></div>"
            "<p><b style='color: #ce9178;'>Время:</b> <span style='color: #858585;'>🕐 %5</span></p>"
            "<p><b style='color: #ce9178;'>Статус:</b> <span style='color: %6;'>%7 %8</span></p></div>")
        .arg(message->id).arg(message->sender_id).arg(receiverStr)
        .arg(QString::fromStdString(message->getContent()).toHtmlEscaped())
        .arg(timestamp.toString("dd.MM.yyyy HH:mm:ss"))
        .arg(statusColor).arg(statusIcon).arg(statusText)
        .arg(message->type == MESSAGE_BROADCAST ? "📢 Рассылка" : "Обычное"));
}

void WidgetManager::onContactAdded(const Contact& contact)
{
    Q_UNUSED(contact);
    updateContacts();
}

void WidgetManager::onContactRemoved(contact_id contactId)
{
    Q_UNUSED(contactId);
    updateContacts();
}

void WidgetManager::onChatAdded(std::shared_ptr<Chat> chat)
{
    Q_UNUSED(chat);
    updateChats();
}

void WidgetManager::onChatRemoved(chat_id chatId)
{
    if (chatId == m_selectedChatId) {
        m_selectedChatId = -1;
        updateMessagesForChat(-1);
        updateSendButtonState();
    }
    updateChats();
}

void WidgetManager::onMessageAdded(std::shared_ptr<Message<std::string>> message)
{
    updateChats();
    if (message && m_selectedChatId >= 0) {
        if (message->type == MESSAGE_BROADCAST || message->receiver_id == m_selectedChatId)
            updateMessagesForChat(m_selectedChatId);
    }
}

void WidgetManager::onMessageRemoved(message_id messageId)
{
    Q_UNUSED(messageId);
    updateChats();
    if (m_selectedChatId >= 0)
        updateMessagesForChat(m_selectedChatId);
}

void WidgetManager::clearAll()
{
    m_contactsTree->clear();
    m_chatsTree->clear();
    m_messagesTable->clearContents();
    m_messagesTable->setRowCount(0);
    m_messageDetails->clear();
}
