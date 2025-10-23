#ifndef CONTACT_H
#define CONTACT_H

#include "types.h"
#include <string>
#include <chrono>

class Contact {
    private:
        contact_id id;
        user_id ownerId;
        user_id contactId;
        std::string contactName;
        std::chrono::system_clock::time_point addedDate;
    public:
        Contact();
        ~Contact();
        contact_id getId();
        user_id getOwnerId();
        void setOwnerId(user_id id);
        user_id getContactId();
        void setContactId(user_id id);
        std::string getContactName();
        void setContactName(std::string name);
        std::chrono::system_clock getAddedDate();
        void setAddedDate(std::chrono::system_clock date);
};

#endif
