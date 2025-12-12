#ifndef CONTACT_H
#define CONTACT_H

#include "types.h"
#include <QString>
#include <QDateTime>

class Contact {
public:
    contact_id id;
    user_id ownerId;
    user_id contactId;
    QString contactName;
    QDateTime addedDate;


    Contact(contact_id id, user_id ownerId, user_id contactId, QString contactName, QDateTime addedDate);
    Contact(contact_id id, user_id ownerId, user_id contactId, QString contactName);
    ~Contact() = default;
};

#endif