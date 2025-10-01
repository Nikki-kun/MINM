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
        std::chrono::system_clock addedDate;
    public:
        contact_id getId();
};

#endif
