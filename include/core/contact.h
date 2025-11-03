#ifndef CONTACT_H
#define CONTACT_H

#include "types.h"
#include <QString>
#include <QDateTime>

class Contact {
private:
    contact_id id;
    user_id ownerId;
    user_id contactId;
    QString contactName;
    QDateTime addedDate;

public:
    Contact(contact_id id, user_id ownerId, user_id contactId, QString contactName, QDateTime addedDate);
    Contact(contact_id id, user_id ownerId, user_id contactId, QString contactName);
    ~Contact() = default;

    contact_id getId() const;
    user_id getOwnerId() const;
    user_id getContactId() const;
    QString getContactName() const;
    QDateTime getAddedDate() const;

    void setContactId(user_id id);
    void setContactName(QString name);
    void setAddedDate(QDateTime date);

    bool isValid() const;
    static bool validateContactName(const QString& contactName);
};

#endif