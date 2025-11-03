#include "core/contact.h"
#include <QDebug>

Contact::Contact(contact_id id, user_id ownerId, user_id contactId, 
                 QString contactName, QDateTime addedDate)
    : id(id), ownerId(ownerId), contactId(contactId), 
      contactName(contactName), addedDate(addedDate) {
    
    if (!validateContactName(contactName)) {
        qWarning() << "Имя контакта не может быть пустым или превышать" << MAX_CONTACT_NAME_LENGTH << "символов:" << contactName;
        this->contactName = "InvalidContact";
    }
    if (ownerId == contactId) {
        qWarning() << "Владелец не может быть своим же контактом";
        // Можно сгенерировать исключение или оставить как есть с предупреждением
    }
}

Contact::Contact(contact_id id, user_id ownerId, user_id contactId, QString contactName)
    : Contact(id, ownerId, contactId, contactName, QDateTime::currentDateTime()) {
}

contact_id Contact::getId() const {
    return id;
}

user_id Contact::getOwnerId() const {
    return ownerId;
}

user_id Contact::getContactId() const {
    return contactId;
}

QString Contact::getContactName() const {
    return contactName;
}

QDateTime Contact::getAddedDate() const {
    return addedDate;
}

void Contact::setContactId(user_id id) {
    if (id == ownerId) {
        qWarning() << "Контакт не может быть тем же пользователем, что и владелец";
        return;
    }
    contactId = id;
}

void Contact::setContactName(QString name) {
    if (!validateContactName(name)) {
        qWarning() << "Имя контакта не может быть пустым или превышать" << MAX_CONTACT_NAME_LENGTH << "символов:" << name;
        return;
    }
    contactName = name;
}

void Contact::setAddedDate(QDateTime date) {
    addedDate = date;
}

bool Contact::isValid() const {
    return validateContactName(contactName) && ownerId != contactId;
}

bool Contact::validateContactName(const QString& contactName) {
    return !contactName.isEmpty() && contactName.length() <= MAX_CONTACT_NAME_LENGTH;
}