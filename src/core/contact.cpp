#include "core/contact.h"

Contact::Contact(contact_id id, user_id ownerId, user_id contactId, 
                 std::string contactName, std::chrono::system_clock::time_point addedDate)
    : id(id), ownerId(ownerId), contactId(contactId), 
      contactName(contactName), addedDate(addedDate) {
    
    if (!validateContactName(contactName)) {
        throw std::invalid_argument("Имя контакта не может быть пустым или превышать " + std::to_string(MAX_CONTACT_NAME_LENGTH) + " символов");
    }
    if (ownerId == contactId) {
        throw std::invalid_argument("Владелец не может быть своим же контактом");
    }
}

Contact::Contact(contact_id id, user_id ownerId, user_id contactId, std::string contactName)
    : Contact(id, ownerId, contactId, contactName, std::chrono::system_clock::now()) {
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

std::string Contact::getContactName() const {
    return contactName;
}

std::chrono::system_clock::time_point Contact::getAddedDate() const {
    return addedDate;
}

void Contact::setContactId(user_id id) {
    if (id == ownerId) {
        throw std::invalid_argument("Контакт не может быть тем же пользователем, что и владелец");
    }
    contactId = id;
}

void Contact::setContactName(std::string name) {
    if (!validateContactName(name)) {
        throw std::invalid_argument("Имя контакта не может быть пустым или превышать " + std::to_string(MAX_CONTACT_NAME_LENGTH) + " символов");
    }
    contactName = name;
}

void Contact::setAddedDate(std::chrono::system_clock::time_point date) {
    addedDate = date;
}

bool Contact::isValid() const {
    return validateContactName(contactName) && ownerId != contactId;
}

bool Contact::validateContactName(const std::string& contactName) {
    return !contactName.empty() && contactName.length() <= MAX_CONTACT_NAME_LENGTH;
}