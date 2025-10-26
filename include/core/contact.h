#ifndef CONTACT_H
#define CONTACT_H

#include "types.h"

class Contact {
private:
    contact_id id;
    user_id ownerId;
    user_id contactId;
    std::string contactName;
    std::chrono::system_clock::time_point addedDate;

public:
    Contact(contact_id id, user_id ownerId, user_id contactId, std::string contactName, std::chrono::system_clock::time_point addedDate);
    Contact(contact_id id, user_id ownerId, user_id contactId, std::string contactName);
    ~Contact() = default;

    contact_id getId() const;
    user_id getOwnerId() const;
    user_id getContactId() const;
    std::string getContactName() const;
    std::chrono::system_clock::time_point getAddedDate() const;

    void setContactId(user_id id);
    void setContactName(std::string name);
    void setAddedDate(std::chrono::system_clock::time_point date);

    bool isValid() const;
    static bool validateContactName(const std::string& contactName);
};

#endif