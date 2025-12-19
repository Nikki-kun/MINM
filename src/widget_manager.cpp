// widget_manager.cpp
#include "widget_manager.h"
#include <QHeaderView>
#include <QDateTime>
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
    
    setMinimumSize(1200, 700);
    setWindowTitle("Messenger Manager");
}

WidgetManager::~WidgetManager()
{
    clearAll();
}

void WidgetManager::setupUI()
{
    // Создаем основной сплиттер
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Левая панель: Контакты
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    m_contactsLabel = new QLabel("Контакты", leftPanel);
    m_contactsLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    m_contactsTree = new QTreeWidget(leftPanel);
    m_contactsTree->setHeaderLabel("Контакты");
    m_contactsTree->setColumnCount(1);
    m_contactsTree->setRootIsDecorated(false);
    m_contactsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    
    leftLayout->addWidget(m_contactsLabel);
    leftLayout->addWidget(m_contactsTree);
    leftPanel->setLayout(leftLayout);
    
    // Центральная панель: Чаты
    QWidget *centerPanel = new QWidget();
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
    
    m_chatsLabel = new QLabel("Чаты", centerPanel);
    m_chatsLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    m_chatsTree = new QTreeWidget(centerPanel);
    m_chatsTree->setHeaderLabels({"ID", "Тип", "Участники", "Сообщения", "Создан"});
    m_chatsTree->setRootIsDecorated(false);
    m_chatsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_chatsTree->header()->setStretchLastSection(true);
    
    centerLayout->addWidget(m_chatsLabel);
    centerLayout->addWidget(m_chatsTree);
    centerPanel->setLayout(centerLayout);
    
    // Правая панель: Сообщения и детали
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical, rightPanel);
    
    // Верхняя часть: Сообщения
    QWidget *messagesWidget = new QWidget();
    QVBoxLayout *messagesLayout = new QVBoxLayout(messagesWidget);
    
    m_messagesLabel = new QLabel("Сообщения", messagesWidget);
    m_messagesLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    m_messagesTable = new QTableWidget(messagesWidget);
    m_messagesTable->setColumnCount(5);
    m_messagesTable->setHorizontalHeaderLabels({"ID", "Отправитель", "Получатель", "Контент", "Время"});
    m_messagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_messagesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_messagesTable->horizontalHeader()->setStretchLastSection(true);
    
    messagesLayout->addWidget(m_messagesLabel);
    messagesLayout->addWidget(m_messagesTable);
    messagesWidget->setLayout(messagesLayout);
    
    // Нижняя часть: Детали сообщения
    QWidget *detailsWidget = new QWidget();
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsWidget);
    
    m_detailsLabel = new QLabel("Детали сообщения", detailsWidget);
    m_detailsLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    m_messageDetails = new QTextEdit(detailsWidget);
    m_messageDetails->setReadOnly(true);
    m_messageDetails->setMaximumHeight(150);
    
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
    mainLayout->addWidget(m_mainSplitter);
    setLayout(mainLayout);
}

void WidgetManager::setupConnections()
{
    connect(m_contactsTree, &QTreeWidget::itemClicked, 
            [this](QTreeWidgetItem *item, int column) {
                Q_UNUSED(column);
                int index = m_contactsTree->indexOfTopLevelItem(item);
                if (index >= 0) {
                    onContactSelected(index);
                }
            });
    
    connect(m_chatsTree, &QTreeWidget::itemClicked, 
            [this](QTreeWidgetItem *item, int column) {
                Q_UNUSED(column);
                int index = m_chatsTree->indexOfTopLevelItem(item);
                if (index >= 0) {
                    onChatSelected(index);
                }
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
        item->setText(0, QString("%1 (ID: %2)").arg(contact.contactName).arg(contact.contactId));
        item->setData(0, Qt::UserRole, QVariant::fromValue(contact.id));
        
        // Добавляем подсказку
        item->setToolTip(0, QString("Владелец: %1\nДобавлен: %2")
                         .arg(contact.ownerId)
                         .arg(contact.addedDate.toString("dd.MM.yyyy HH:mm")));
    }
    
    m_contactsTree->resizeColumnToContents(0);
}

void WidgetManager::updateChats()
{
    m_chatsTree->clear();
    
    for (const auto& chat : m_chats) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_chatsTree);
        
        item->setText(0, QString::number(chat->id));
        
        QString typeStr = (chat->type == chat_type::PRIVATE) ? "Приватный" : "Групповой";
        item->setText(1, typeStr);
        
        item->setText(2, QString::number(chat->getParticipants().size()));
        item->setText(3, QString::number(chat->getMessages().size()));
        
        // Конвертируем время из chrono в QDateTime
        auto timePoint = chat->created_date;
        auto duration = timePoint.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        QDateTime created = QDateTime::fromSecsSinceEpoch(seconds);
        item->setText(4, created.toString("dd.MM.yyyy HH:mm"));
        
        item->setData(0, Qt::UserRole, QVariant::fromValue(chat->id));
    }
    
    for (int i = 0; i < 5; i++) {
        m_chatsTree->resizeColumnToContents(i);
    }
}

void WidgetManager::updateMessages()
{
    m_messagesTable->clearContents();
    m_messagesTable->setRowCount(m_messages.size());
    
    for (int i = 0; i < m_messages.size(); i++) {
        const auto& message = m_messages[i];
        
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(message->id));
        idItem->setData(Qt::UserRole, QVariant::fromValue(message->id));
        
        QTableWidgetItem *senderItem = new QTableWidgetItem(QString::number(message->sender_id));
        QTableWidgetItem *receiverItem = new QTableWidgetItem(QString::number(message->receiver_id));
        QTableWidgetItem *contentItem = new QTableWidgetItem(QString::fromStdString(message->getContent()));
        
        // Конвертируем время
        auto timePoint = message->timestamp;
        auto duration = timePoint.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(milliseconds);
        QTableWidgetItem *timeItem = new QTableWidgetItem(timestamp.toString("dd.MM.yyyy HH:mm:ss.zzz"));
        
        m_messagesTable->setItem(i, 0, idItem);
        m_messagesTable->setItem(i, 1, senderItem);
        m_messagesTable->setItem(i, 2, receiverItem);
        m_messagesTable->setItem(i, 3, contentItem);
        m_messagesTable->setItem(i, 4, timeItem);
    }
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
        emit contactSelected(contact.id);
        
        // Показываем детали контакта
        m_messageDetails->setText(
            QString("Детали контакта:\n"
                    "ID: %1\n"
                    "Владелец: %2\n"
                    "Контакт ID: %3\n"
                    "Имя: %4\n"
                    "Добавлен: %5")
            .arg(contact.id)
            .arg(contact.ownerId)
            .arg(contact.contactId)
            .arg(contact.contactName)
            .arg(contact.addedDate.toString("dd.MM.yyyy HH:mm:ss"))
        );
    }
}

void WidgetManager::onChatSelected(int index)
{
    if (index >= 0 && index < m_chats.size()) {
        const auto& chat = m_chats[index];
        emit chatSelected(chat->id);
        
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
        
        // Показываем детали чата
        m_messageDetails->setText(
            QString("Детали чата:\n"
                    "ID: %1\n"
                    "Тип: %2\n"
                    "Участников: %3\n"
                    "Сообщений: %4\n"
                    "Участники: %5\n"
                    "Сообщения ID: %6")
            .arg(chat->id)
            .arg(chat->type == chat_type::PRIVATE ? "Приватный" : "Групповой")
            .arg(participants.size())
            .arg(messages.size())
            .arg(participantsStr)
            .arg(messagesStr)
        );
    }
}

void WidgetManager::onMessageSelected(int index)
{
    if (index >= 0 && index < m_messages.size()) {
        const auto& message = m_messages[index];
        emit messageSelected(message->id);
        
        // Конвертируем время
        auto timePoint = message->timestamp;
        auto duration = timePoint.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(milliseconds);
        
        // Показываем детали сообщения
        m_messageDetails->setText(
            QString("Детали сообщения:\n"
                    "ID: %1\n"
                    "Отправитель: %2\n"
                    "Получатель (чат): %3\n"
                    "Контент: %4\n"
                    "Время: %5\n"
                    "Статус: %6")
            .arg(message->id)
            .arg(message->sender_id)
            .arg(message->receiver_id)
            .arg(QString::fromStdString(message->getContent()))
            .arg(timestamp.toString("dd.MM.yyyy HH:mm:ss.zzz"))
            .arg(message->getStatus() == SENT ? "Отправлено" : 
                 message->getStatus() == DELIVERED ? "Доставлено" : "Прочитано")
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
    Q_UNUSED(chatId);
    updateChats();
}

void WidgetManager::onMessageAdded(std::shared_ptr<Message<std::string>> message)
{
    Q_UNUSED(message);
    updateMessages();
}

void WidgetManager::onMessageRemoved(message_id messageId)
{
    Q_UNUSED(messageId);
    updateMessages();
}

void WidgetManager::clearAll()
{
    m_contactsTree->clear();
    m_chatsTree->clear();
    m_messagesTable->clearContents();
    m_messagesTable->setRowCount(0);
    m_messageDetails->clear();
}