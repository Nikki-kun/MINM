#ifndef WIDGET_MANAGER_H
#define WIDGET_MANAGER_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVector>
#include <QTreeWidget>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLineEdit>
#include <QComboBox>
#include "core/contact.h"
#include "core/сhat.h"
#include "core/message.h"

class MessageManager;

class WidgetManager : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetManager(
        MessageManager& manager,
        QVector<Contact>& contacts,
        QVector<std::shared_ptr<Chat>>& chats,
        QVector<std::shared_ptr<Message<std::string>>>& messages,
        QWidget *parent = nullptr);
    
    ~WidgetManager();
    
public slots:
    void updateContacts();
    void updateChats();
    void updateMessages();
    void updateAll();
    
    void onContactSelected(int index);
    void onChatSelected(chat_id chatId);
    void onMessageSelected(int index);
    
    void onContactAdded(const Contact& contact);
    void onContactRemoved(contact_id contactId);
    void onChatAdded(std::shared_ptr<Chat> chat);
    void onChatRemoved(chat_id chatId);
    void onMessageAdded(std::shared_ptr<Message<std::string>> message);
    void onMessageRemoved(message_id messageId);

signals:
    void contactSelected(contact_id id);
    void chatSelected(chat_id id);
    void messageSelected(message_id id);

private:
    void setupUI();
    void setupConnections();
    void clearAll();
    void updateMessagesForChat(chat_id chatId);
    void updateSendButtonState();
    QString roleToLabel(chat_participant_role role) const;
    QString statusToLabel(chat_participant_status status) const;

    MessageManager& m_manager;
    static constexpr user_id CURRENT_USER_ID = 0;
    user_id m_selectedContactUserId = -1;
    chat_id m_selectedChatId = -1;

    QVector<Contact>& m_contacts;
    QVector<std::shared_ptr<Chat>>& m_chats;
    QVector<std::shared_ptr<Message<std::string>>>& m_messages;
    
    QSplitter *m_mainSplitter;
    
    QTreeWidget *m_contactsTree;
    QTreeWidget *m_chatsTree;
    QTableWidget *m_messagesTable;
    QTextEdit *m_messageDetails;
    
    QPushButton *m_contactsLabel;
    QLabel *m_chatsLabel;
    QLabel *m_messagesLabel;
    QLabel *m_detailsLabel;

    QLineEdit *m_messageInput;
    QPushButton *m_sendButton;
    QLineEdit *m_participantIdInput;
    QComboBox *m_roleCombo;
    QPushButton *m_setRoleButton;
    QPushButton *m_banButton;
    QPushButton *m_leaveButton;
    QPushButton *m_activateButton;
};

#endif // WIDGET_MANAGER_H