#ifndef USER_H
#define USER_H

#include "types.h"
#include "contact.h"

typedef struct User {
    user_id user_id;
    char username[MAX_USERNAME_LEN];
    char password_hash[MAX_PASSWORD_LEN];
    bool online_status;
    time_t last_seen;
    list* contacts;
    list* blocked_users;
} user;

#endif