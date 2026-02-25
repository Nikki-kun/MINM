#include "widget_manager.h"
#include "managers/message_manager.h"
#include "core/types.h"
#include "ui/styles.h"
#include <QHeaderView>
#include <QJsonObject>
#include <algorithm>

WidgetManager::WidgetManager(MessageManager& manager,
    QVector<Contact>& contacts,
    QVector<std::shared_ptr<Chat>>& chats,
    QVector<std::shared_ptr<Message<std::string>>>& messages,
    QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
    , m_contacts(contacts)
    , m_chats(chats)
    , m_messages(messages)
{
    setupUI();
    setupConnections();
    updateAll();
    setMinimumSize(1400, 800);
    setWindowTitle("💬 Messenger Manager");
}

WidgetManager::~WidgetManager()
{
    clearAll();
}

void WidgetManager::setupUI()
{
    setStyleSheet(Ui::mainStylesheet());
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(4);

    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(8);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    m_contactsLabel = new QPushButton("👥 Контакты", leftPanel);
    m_contactsLabel->setToolTip("Нажмите, чтобы показать все чаты");
    m_contactsLabel->setFlat(true);
    m_contactsLabel->setCursor(Qt::PointingHandCursor);
    m_contactsLabel->setStyleSheet(Ui::contactsButtonStyle());
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

    QWidget *centerPanel = new QWidget();
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(8);
    centerLayout->setContentsMargins(12, 12, 12, 12);
    m_chatsLabel = new QLabel("💬 Чаты", centerPanel);
    m_chatsLabel->setStyleSheet(Ui::panelHeaderStyle());
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

    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(8);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical, rightPanel);
    rightSplitter->setHandleWidth(4);

    QWidget *messagesWidget = new QWidget();
    QVBoxLayout *messagesLayout = new QVBoxLayout(messagesWidget);
    messagesLayout->setSpacing(8);
    messagesLayout->setContentsMargins(0, 0, 0, 0);
    m_messagesLabel = new QLabel("📨 Сообщения", messagesWidget);
    m_messagesLabel->setStyleSheet(Ui::panelHeaderStyle());
    m_messagesTable = new QTableWidget(messagesWidget);
    m_messagesTable->setColumnCount(5);
    m_messagesTable->setHorizontalHeaderLabels({"ID", "Отправитель", "Получатель", "Контент", "Время"});
    m_messagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_messagesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_messagesTable->horizontalHeader()->setStretchLastSection(true);
    m_messagesTable->setAlternatingRowColors(true);
    m_messagesTable->setShowGrid(false);
    m_messagesTable->verticalHeader()->setVisible(false);
    m_messageInput = new QLineEdit(messagesWidget);
    m_messageInput->setPlaceholderText("Введите сообщение...");
    m_messageInput->setMaxLength(MAX_MESSAGE_LENGTH);
    m_sendButton = new QPushButton("📤 Отправить", messagesWidget);
    m_sendButton->setEnabled(false);
    QHBoxLayout *sendLayout = new QHBoxLayout();
    sendLayout->addWidget(m_messageInput, 1);
    sendLayout->addWidget(m_sendButton, 0);
    messagesLayout->addWidget(m_messagesLabel);
    messagesLayout->addWidget(m_messagesTable);
    messagesLayout->addLayout(sendLayout);
    messagesWidget->setLayout(messagesLayout);

    QWidget *detailsWidget = new QWidget();
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setSpacing(8);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    m_detailsLabel = new QLabel("ℹ️ Детали", detailsWidget);
    m_detailsLabel->setStyleSheet(Ui::panelHeaderStyle());
    m_messageDetails = new QTextEdit(detailsWidget);
    m_messageDetails->setReadOnly(true);
    m_messageDetails->setMaximumHeight(150);
    m_messageDetails->setStyleSheet(Ui::detailsTextEditStyle());
    detailsLayout->addWidget(m_detailsLabel);
    detailsLayout->addWidget(m_messageDetails);
    detailsWidget->setLayout(detailsLayout);

    rightSplitter->addWidget(messagesWidget);
    rightSplitter->addWidget(detailsWidget);
    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 1);
    rightLayout->addWidget(rightSplitter);
    rightPanel->setLayout(rightLayout);

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
        m_selectedChatId = -1;
        m_chatsTree->clearSelection();
        updateChats();
        updateMessagesForChat(-1);
        m_messageDetails->clear();
        updateSendButtonState();
    });
    connect(m_contactsTree, &QTreeWidget::itemClicked, [this](QTreeWidgetItem *item, int column) {
        Q_UNUSED(column);
        contact_id contactId = item->data(0, Qt::UserRole).value<contact_id>();
        auto it = std::find_if(m_contacts.begin(), m_contacts.end(), [contactId](const Contact& c) { return c.id == contactId; });
        if (it != m_contacts.end())
            onContactSelected(std::distance(m_contacts.begin(), it));
    });
    connect(m_chatsTree, &QTreeWidget::itemClicked, [this](QTreeWidgetItem *item, int column) {
        Q_UNUSED(column);
        onChatSelected(item->data(0, Qt::UserRole).value<chat_id>());
    });
    connect(m_messagesTable, &QTableWidget::itemClicked, [this](QTableWidgetItem *item) {
        onMessageSelected(item->row());
    });
    connect(m_sendButton, &QPushButton::clicked, [this]() {
        if (m_selectedChatId < 0) return;
        QString text = m_messageInput->text().trimmed();
        if (text.isEmpty()) return;
        QJsonObject data;
        data["sender_id"] = static_cast<qint64>(CURRENT_USER_ID);
        data["receiver_id"] = static_cast<qint64>(m_selectedChatId);
        data["content"] = text;
        data["type"] = MESSAGE_NORMAL;
        if (m_manager.addMessage(data))
            m_messageInput->clear();
    });
    connect(m_messageInput, &QLineEdit::returnPressed, [this]() { m_sendButton->animateClick(); });
}
