#ifndef WIDGET_MANAGER_H
#define WIDGET_MANAGER_H

#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QVector>
#include <QTreeWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include "core/contact.h"
#include "core/сhat.h"
#include "core/message.h"

class WidgetManager : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetManager(
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
    void onChatSelected(int index);
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

    chat_id m_selectedChatId = -1;

    QVector<Contact>& m_contacts;
    QVector<std::shared_ptr<Chat>>& m_chats;
    QVector<std::shared_ptr<Message<std::string>>>& m_messages;
    
    QSplitter *m_mainSplitter;
    
    QTreeWidget *m_contactsTree;
    QTreeWidget *m_chatsTree;
    QTableWidget *m_messagesTable;
    QTextEdit *m_messageDetails;
    
    QLabel *m_contactsLabel;
    QLabel *m_chatsLabel;
    QLabel *m_messagesLabel;
    QLabel *m_detailsLabel;
};

#endif // WIDGET_MANAGER_H