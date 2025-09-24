#ifndef CONTACT_H
#define CONTACT_H

#include "types.h"

typedef struct {
    user_id owner_id;
    user_id contact_id;
    char contact_name[MAX_CONTACT_NAME_LEN];
    time_t added_date;
} contact;

#endif