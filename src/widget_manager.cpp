// widget_manager.cpp
#include "widget_manager.h"
#include <QHeaderView>
#include <QDateTime>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

WidgetManager::WidgetManager(
    QVector<Contact>& contacts,
    QVector<std::shared_ptr<Chat>>& chats,
    QVector<std::shared_ptr<Message<std::string>>>& messages,
    QWidget *parent)
    : QWidget(parent)
    , m_contacts(contacts)
    , m_chats(chats)
    , m_messages(messages)
{
    setupUI();
    setupConnections();
    updateAll();
    
    setMinimumSize(1400, 800);
    setWindowTitle("💬 Messenger Manager - Современный интерфейс");
}

WidgetManager::~WidgetManager()
{
    clearAll();
}

void WidgetManager::setupUI()
{
    // Применяем современную темную тему
    setStyleSheet(
        "QWidget {"
        "    background-color: #1e1e1e;"
        "    color: #d4d4d4;"
        "    font-family: 'Segoe UI', 'Roboto', 'Arial', sans-serif;"
        "    font-size: 10pt;"
        "}"
        "QLabel {"
        "    color: #ffffff;"
        "    padding: 8px;"
        "    background-color: #252526;"
        "    border-radius: 4px;"
        "}"
        "QTreeWidget {"
        "    background-color: #252526;"
        "    border: 1px solid #3e3e42;"
        "    border-radius: 6px;"
        "    padding: 4px;"
        "    selection-background-color: #0078d4;"
        "    selection-color: #ffffff;"
        "    alternate-background-color: #2d2d30;"
        "}"
        "QTreeWidget::item {"
        "    padding: 6px;"
        "    border-radius: 3px;"
        "    min-height: 24px;"
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: #2a2d2e;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: #0078d4;"
        "    color: #ffffff;"
        "}"
        "QTreeWidget::branch {"
        "    background-color: #252526;"
        "}"
        "QHeaderView::section {"
        "    background-color: #2d2d30;"
        "    color: #ffffff;"
        "    padding: 8px;"
        "    border: none;"
        "    border-bottom: 2px solid #0078d4;"
        "    font-weight: bold;"
        "    font-size: 10pt;"
        "}"
        "QTableWidget {"
        "    background-color: #252526;"
        "    border: 1px solid #3e3e42;"
        "    border-radius: 6px;"
        "    gridline-color: #3e3e42;"
        "    selection-background-color: #0078d4;"
        "    selection-color: #ffffff;"
        "    alternate-background-color: #2d2d30;"
        "}"
        "QTableWidget::item {"
        "    padding: 6px;"
        "    border: none;"
        "}"
        "QTableWidget::item:hover {"
        "    background-color: #2a2d2e;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #0078d4;"
        "    color: #ffffff;"
        "}"
        "QTextEdit {"
        "    background-color: #252526;"
        "    border: 1px solid #3e3e42;"
        "    border-radius: 6px;"
        "    padding: 8px;"
        "    color: #d4d4d4;"
        "    selection-background-color: #0078d4;"
        "    selection-color: #ffffff;"
        "}"
        "QSplitter::handle {"
        "    background-color: #3e3e42;"
        "    width: 4px;"
        "    height: 4px;"
        "}"
        "QSplitter::handle:hover {"
        "    background-color: #0078d4;"
        "}"
    );
    
    // Создаем основной сплиттер
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(4);
    
    // Левая панель: Контакты
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(8);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    
    m_contactsLabel = new QPushButton("👥 Контакты", leftPanel);
    m_contactsLabel->setToolTip("Нажмите, чтобы показать все чаты");
    m_contactsLabel->setFlat(true);
    m_contactsLabel->setCursor(Qt::PointingHandCursor);
    m_contactsLabel->setStyleSheet(
        "QPushButton {"
        "    font-weight: bold;"
        "    font-size: 13pt;"
        "    color: #ffffff;"
        "    padding: 12px;"
        "    background-color: #0078d4;"
        "    border-radius: 6px;"
        "    border: none;"
        "    text-align: left;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106ebe;"
        "}"
    );
    
    m_contactsTree = new QTreeWidget(leftPanel);
    m_contactsTree->setHeaderLabel("Контакты");
    m_contactsTree->setColumnCount(1);
    m_contactsTree->setRootIsDecorated(false);
    m_contactsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_contactsTree->setAlternatingRowColors(true);
    m_contactsTree->setAnimated(true);
    
    leftLayout->addWidget(m_contactsLabel);
    leftLayout->addWidget(m_contactsTree);
    leftPanel->setLayout(leftLayout);
    
    // Центральная панель: Чаты
    QWidget *centerPanel = new QWidget();
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(8);
    centerLayout->setContentsMargins(12, 12, 12, 12);
    
    m_chatsLabel = new QLabel("💬 Чаты", centerPanel);
    m_chatsLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 13pt;"
        "    color: #ffffff;"
        "    padding: 12px;"
        "    background-color: #0078d4;"
        "    border-radius: 6px;"
        "    border: none;"
        "}"
    );
    
    m_chatsTree = new QTreeWidget(centerPanel);
    m_chatsTree->setHeaderLabels({"ID", "Тип", "Участники", "Сообщения", "Создан"});
    m_chatsTree->setRootIsDecorated(false);
    m_chatsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_chatsTree->header()->setStretchLastSection(true);
    m_chatsTree->setAlternatingRowColors(true);
    m_chatsTree->setAnimated(true);
    
    centerLayout->addWidget(m_chatsLabel);
    centerLayout->addWidget(m_chatsTree);
    centerPanel->setLayout(centerLayout);
    
    // Правая панель: Сообщения и детали
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(8);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical, rightPanel);
    rightSplitter->setHandleWidth(4);
    
    // Верхняя часть: Сообщения
    QWidget *messagesWidget = new QWidget();
    QVBoxLayout *messagesLayout = new QVBoxLayout(messagesWidget);
    messagesLayout->setSpacing(8);
    messagesLayout->setContentsMargins(0, 0, 0, 0);
    
    m_messagesLabel = new QLabel("📨 Сообщения", messagesWidget);
    m_messagesLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 13pt;"
        "    color: #ffffff;"
        "    padding: 12px;"
        "    background-color: #0078d4;"
        "    border-radius: 6px;"
        "    border: none;"
        "}"
    );
    
    m_messagesTable = new QTableWidget(messagesWidget);
    m_messagesTable->setColumnCount(5);
    m_messagesTable->setHorizontalHeaderLabels({"ID", "Отправитель", "Получатель", "Контент", "Время"});
    m_messagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_messagesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_messagesTable->horizontalHeader()->setStretchLastSection(true);
    m_messagesTable->setAlternatingRowColors(true);
    m_messagesTable->setShowGrid(false);
    m_messagesTable->verticalHeader()->setVisible(false);
    
    messagesLayout->addWidget(m_messagesLabel);
    messagesLayout->addWidget(m_messagesTable);
    messagesWidget->setLayout(messagesLayout);
    
    // Нижняя часть: Детали сообщения
    QWidget *detailsWidget = new QWidget();
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setSpacing(8);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    
    m_detailsLabel = new QLabel("ℹ️ Детали", detailsWidget);
    m_detailsLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 13pt;"
        "    color: #ffffff;"
        "    padding: 12px;"
        "    background-color: #0078d4;"
        "    border-radius: 6px;"
        "    border: none;"
        "}"
    );
    
    m_messageDetails = new QTextEdit(detailsWidget);
    m_messageDetails->setReadOnly(true);
    m_messageDetails->setMaximumHeight(150);
    m_messageDetails->setStyleSheet(
        "QTextEdit {"
        "    font-family: 'Consolas', 'Monaco', 'Courier New', monospace;"
        "    font-size: 9pt;"
        "    line-height: 1.4;"
        "}"
    );
    
    detailsLayout->addWidget(m_detailsLabel);
    detailsLayout->addWidget(m_messageDetails);
    detailsWidget->setLayout(detailsLayout);
    
    rightSplitter->addWidget(messagesWidget);
    rightSplitter->addWidget(detailsWidget);
    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 1);
    
    rightLayout->addWidget(rightSplitter);
    rightPanel->setLayout(rightLayout);
    
    // Добавляем все панели в основной сплиттер
    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(centerPanel);
    m_mainSplitter->addWidget(rightPanel);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 2);
    m_mainSplitter->setStretchFactor(2, 3);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_mainSplitter);
    setLayout(mainLayout);
}

void WidgetManager::setupConnections()
{
    connect(m_contactsLabel, &QPushButton::clicked, [this]() {
        m_selectedContactUserId = -1;
        m_contactsTree->clearSelection();
        updateChats();
        updateMessagesForChat(-1);
        m_messageDetails->clear();
    });

    connect(m_contactsTree, &QTreeWidget::itemClicked,
            [this](QTreeWidgetItem *item, int column) {
                Q_UNUSED(column);
                contact_id contactId = item->data(0, Qt::UserRole).value<contact_id>();
                auto it = std::find_if(m_contacts.begin(), m_contacts.end(),
                    [contactId](const Contact& c) { return c.id == contactId; });
                if (it != m_contacts.end()) {
                    int index = std::distance(m_contacts.begin(), it);
                    onContactSelected(index);
                }
            });
    
    connect(m_chatsTree, &QTreeWidget::itemClicked,
            [this](QTreeWidgetItem *item, int column) {
                Q_UNUSED(column);
                chat_id chatId = item->data(0, Qt::UserRole).value<chat_id>();
                onChatSelected(chatId);
            });
    
    connect(m_messagesTable, &QTableWidget::itemClicked, 
            [this](QTableWidgetItem *item) {
                int row = item->row();
                onMessageSelected(row);
            });
}

void WidgetManager::updateContacts()
{
    m_contactsTree->clear();

    for (const Contact& contact : m_contacts) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_contactsTree);
        QString displayText = QString("👤 %1").arg(contact.contactName);
        item->setText(0, displayText);
        item->setData(0, Qt::UserRole, QVariant::fromValue(contact.id));
        item->setData(0, Qt::UserRole + 1, QVariant::fromValue(contact.contactId));
        
        // Улучшенная подсказка с форматированием
        item->setToolTip(0, QString("<b>Контакт:</b> %1<br>"
                                    "<b>ID контакта:</b> %2<br>"
                                    "<b>Владелец:</b> %3<br>"
                                    "<b>Добавлен:</b> %4")
                         .arg(contact.contactName)
                         .arg(contact.contactId)
                         .arg(contact.ownerId)
                         .arg(contact.addedDate.toString("dd.MM.yyyy HH:mm")));
        
        // Устанавливаем цвет текста
        item->setForeground(0, QBrush(QColor("#d4d4d4")));
    }
    
    m_contactsTree->resizeColumnToContents(0);
}

void WidgetManager::updateChats()
{
    m_chatsTree->clear();

    for (const auto& chat : m_chats) {
        if (m_selectedContactUserId >= 0) {
            // Контакт выбран — показываем только чаты, где и пользователь (0), и контакт
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
        
        // Конвертируем время из chrono в QDateTime
        auto timePoint = chat->created_date;
        auto duration = timePoint.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        QDateTime created = QDateTime::fromSecsSinceEpoch(seconds);
        item->setText(4, created.toString("dd.MM.yyyy HH:mm"));
        
        item->setData(0, Qt::UserRole, QVariant::fromValue(chat->id));
        
        // Устанавливаем цвета для разных колонок
        item->setForeground(0, QBrush(QColor("#4ec9b0"))); // ID - бирюзовый
        item->setForeground(1, QBrush(QColor(chat->type == chat_type::PRIVATE ? "#ce9178" : "#4ec9b0")));
        item->setForeground(2, QBrush(QColor("#dcdcaa"))); // Участники - желтый
        item->setForeground(3, QBrush(QColor("#dcdcaa"))); // Сообщения - желтый
        item->setForeground(4, QBrush(QColor("#858585"))); // Время - серый
    }
    
    for (int i = 0; i < 5; i++) {
        m_chatsTree->resizeColumnToContents(i);
    }
}

void WidgetManager::updateMessages()
{
    updateMessagesForChat(m_selectedChatId);
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

    // Находим чат и получаем его сообщения
    auto chatIt = std::find_if(m_chats.begin(), m_chats.end(),
        [chatId](const auto& c) { return c->id == chatId; });
    if (chatIt == m_chats.end()) return;

    const auto& messageIds = (*chatIt)->getMessages();

    // Собираем сообщения чата в порядке их ID
    QVector<std::shared_ptr<Message<std::string>>> chatMessages;
    for (message_id msgId : messageIds) {
        auto msgIt = std::find_if(m_messages.begin(), m_messages.end(),
            [msgId](const auto& m) { return m->id == msgId; });
        if (msgIt != m_messages.end()) {
            chatMessages.append(*msgIt);
        }
    }

    m_messagesTable->setRowCount(chatMessages.size());

    for (int i = 0; i < chatMessages.size(); i++) {
        const auto& message = chatMessages[i];
        
        QTableWidgetItem *idItem = new QTableWidgetItem(QString("#%1").arg(message->id));
        idItem->setData(Qt::UserRole, QVariant::fromValue(message->id));
        idItem->setForeground(QBrush(QColor("#4ec9b0"))); // Бирюзовый для ID
        
        QTableWidgetItem *senderItem = new QTableWidgetItem(QString("👤 %1").arg(message->sender_id));
        senderItem->setForeground(QBrush(QColor("#ce9178"))); // Оранжевый для отправителя
        
        QString receiverText = (message->type == MESSAGE_BROADCAST)
            ? QString("📢 Все чаты") : QString("📨 %1").arg(message->receiver_id);
        QTableWidgetItem *receiverItem = new QTableWidgetItem(receiverText);
        receiverItem->setForeground(QBrush(QColor("#569cd6"))); // Синий для получателя
        
        QString content = QString::fromStdString(message->getContent());
        // Ограничиваем длину контента для лучшего отображения
        if (content.length() > 50) {
            content = content.left(47) + "...";
        }
        QTableWidgetItem *contentItem = new QTableWidgetItem(content);
        contentItem->setForeground(QBrush(QColor("#d4d4d4"))); // Светло-серый для контента
        contentItem->setToolTip(QString::fromStdString(message->getContent())); // Полный текст в подсказке
        
        // Конвертируем время
        auto timePoint = message->timestamp;
        auto duration = timePoint.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(milliseconds);
        QTableWidgetItem *timeItem = new QTableWidgetItem(timestamp.toString("dd.MM.yyyy HH:mm:ss"));
        timeItem->setForeground(QBrush(QColor("#858585"))); // Серый для времени
        
        // Делаем элементы нередактируемыми
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        senderItem->setFlags(senderItem->flags() & ~Qt::ItemIsEditable);
        receiverItem->setFlags(receiverItem->flags() & ~Qt::ItemIsEditable);
        contentItem->setFlags(contentItem->flags() & ~Qt::ItemIsEditable);
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
        
        m_messagesTable->setItem(i, 0, idItem);
        m_messagesTable->setItem(i, 1, senderItem);
        m_messagesTable->setItem(i, 2, receiverItem);
        m_messagesTable->setItem(i, 3, contentItem);
        m_messagesTable->setItem(i, 4, timeItem);
    }
    
    // Автоматически подгоняем ширину колонок
    m_messagesTable->resizeColumnsToContents();
}

void WidgetManager::updateAll()
{
    updateContacts();
    updateChats();
    updateMessages();
}

void WidgetManager::onContactSelected(int index)
{
    if (index >= 0 && index < m_contacts.size()) {
        const Contact& contact = m_contacts[index];
        m_selectedContactUserId = contact.contactId;
        m_selectedChatId = -1;
        emit contactSelected(contact.id);
        updateChats();
        updateMessagesForChat(-1);
        
        // Показываем детали контакта с улучшенным форматированием
        m_messageDetails->setHtml(
            QString("<div style='font-family: Consolas, Monaco, monospace; line-height: 1.6;'>"
                    "<h3 style='color: #4ec9b0; margin-top: 0;'>👤 Детали контакта</h3>"
                    "<p><b style='color: #ce9178;'>ID:</b> <span style='color: #4ec9b0;'>%1</span></p>"
                    "<p><b style='color: #ce9178;'>Владелец:</b> <span style='color: #dcdcaa;'>%2</span></p>"
                    "<p><b style='color: #ce9178;'>Контакт ID:</b> <span style='color: #4ec9b0;'>%3</span></p>"
                    "<p><b style='color: #ce9178;'>Имя:</b> <span style='color: #d4d4d4;'>%4</span></p>"
                    "<p><b style='color: #ce9178;'>Добавлен:</b> <span style='color: #858585;'>%5</span></p>"
                    "</div>")
            .arg(contact.id)
            .arg(contact.ownerId)
            .arg(contact.contactId)
            .arg(contact.contactName)
            .arg(contact.addedDate.toString("dd.MM.yyyy HH:mm:ss"))
        );
    }
}

void WidgetManager::onChatSelected(chat_id chatId)
{
    auto it = std::find_if(m_chats.begin(), m_chats.end(),
        [chatId](const auto& c) { return c->id == chatId; });
    if (it != m_chats.end()) {
        const auto& chat = *it;
        m_selectedChatId = chat->id;
        emit chatSelected(chat->id);
        updateMessagesForChat(chat->id);
        
        // Получаем информацию о чате
        auto participants = chat->getParticipants();
        auto messages = chat->getMessages();
        
        QString participantsStr;
        for (size_t i = 0; i < participants.size(); i++) {
            participantsStr += QString::number(participants[i]);
            if (i < participants.size() - 1) participantsStr += ", ";
        }
        
        QString messagesStr;
        for (size_t i = 0; i < messages.size(); i++) {
            messagesStr += QString::number(messages[i]);
            if (i < messages.size() - 1) messagesStr += ", ";
        }
        
        // Показываем детали чата с улучшенным форматированием
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
                    "<p><b style='color: #ce9178;'>Сообщения ID:</b> <span style='color: #858585;'>%6</span></p>"
                    "</div>")
            .arg(chat->id)
            .arg(typeText)
            .arg(participants.size())
            .arg(messages.size())
            .arg(participantsStr)
            .arg(messagesStr)
            .arg(chat->type == chat_type::PRIVATE ? "#ce9178" : "#4ec9b0")
            .arg(typeIcon)
        );
    }
}

void WidgetManager::onMessageSelected(int row)
{
    if (row < 0) return;
    QTableWidgetItem *idItem = m_messagesTable->item(row, 0);
    if (!idItem) return;

    message_id msgId = idItem->data(Qt::UserRole).value<message_id>();
    auto it = std::find_if(m_messages.begin(), m_messages.end(),
        [msgId](const auto& m) { return m->id == msgId; });
    if (it != m_messages.end()) {
        const auto& message = *it;
        emit messageSelected(message->id);
        
        // Конвертируем время
        auto timePoint = message->timestamp;
        auto duration = timePoint.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(milliseconds);
        
        // Показываем детали сообщения с улучшенным форматированием
        QString statusText;
        QString statusColor;
        QString statusIcon;
        if (message->getStatus() == SENT) {
            statusText = "Отправлено";
            statusColor = "#858585";
            statusIcon = "📤";
        } else if (message->getStatus() == DELIVERED) {
            statusText = "Доставлено";
            statusColor = "#dcdcaa";
            statusIcon = "✓";
        } else {
            statusText = "Прочитано";
            statusColor = "#4ec9b0";
            statusIcon = "✓✓";
        }
        
        QString receiverStr = (message->type == MESSAGE_BROADCAST)
            ? QString("📢 Все чаты (рассылка)") : QString("📨 %1").arg(message->receiver_id);
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
                    "<p><b style='color: #ce9178;'>Статус:</b> <span style='color: %6;'>%7 %8</span></p>"
                    "</div>")
            .arg(message->id)
            .arg(message->sender_id)
            .arg(receiverStr)
            .arg(QString::fromStdString(message->getContent()).toHtmlEscaped())
            .arg(timestamp.toString("dd.MM.yyyy HH:mm:ss"))
            .arg(statusColor)
            .arg(statusIcon)
            .arg(statusText)
            .arg(message->type == MESSAGE_BROADCAST ? "📢 Рассылка" : "Обычное")
        );
    }
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
    }
    updateChats();
}

void WidgetManager::onMessageAdded(std::shared_ptr<Message<std::string>> message)
{
    updateChats();  // Обновляем счётчик сообщений в таблице чатов
    if (message && m_selectedChatId >= 0) {
        if (message->type == MESSAGE_BROADCAST || message->receiver_id == m_selectedChatId) {
            updateMessagesForChat(m_selectedChatId);
        }
    }
}

void WidgetManager::onMessageRemoved(message_id messageId)
{
    Q_UNUSED(messageId);
    updateChats();  // Обновляем счётчик сообщений в таблице чатов
    if (m_selectedChatId >= 0) {
        updateMessagesForChat(m_selectedChatId);  // Обновляем список (в т.ч. для рассылки)
    }
}

void WidgetManager::clearAll()
{
    m_contactsTree->clear();
    m_chatsTree->clear();
    m_messagesTable->clearContents();
    m_messagesTable->setRowCount(0);
    m_messageDetails->clear();
}