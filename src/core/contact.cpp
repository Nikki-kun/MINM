#include "contact.h"

Contact::Contact(contact_id id, user_id ownerId, user_id contactId, 
                 QString contactName, QDateTime addedDate)
    : id(id), ownerId(ownerId), contactId(contactId), 
      contactName(contactName), addedDate(addedDate) {}

Contact::Contact(contact_id id, user_id ownerId, user_id contactId, 
                 QString contactName)
    : id(id), ownerId(ownerId), contactId(contactId), 
      contactName(contactName), addedDate(QDateTime::currentDateTime()) {}