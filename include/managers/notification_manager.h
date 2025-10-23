#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include "../core/user.h"
#include "../core/message.h"
#include <unordered_map>
#include <vector>

class NotificationManager {
private:
    std::unordered_map<user_id, std::vector<User*>*> observers;

public:
    NotificationManager();
    ~NotificationManager();
    
    void notifyNewMessage(user_id receiver_id, Message* message);
    void notifyContactOnline(user_id id, user_id contact_id);
    
    void addObserver(user_id user_id, User* observer);
    void removeObserver(user_id user_id, User* observer);
    
    NotificationManager& operator+(User* observer);
    NotificationManager& operator-(User* observer);
    NotificationManager& operator+(const std::vector<User*>& observers_list);
    NotificationManager& operator-(const std::vector<User*>& observers_list);
};

#endif