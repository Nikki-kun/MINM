#ifndef CONTACT_MANAGER_H
#define CONTACT_MANAGER_H


#include <unordered_map>
#include <vector>
#include <string>
#include <../core/types.h>
#include <../core/contact.h>

class ContactManager {
private:
    std::unordered_map<user_id, std::vector<Contact>*> contacts;

public:
    ContactManager();
    ~ContactManager();
    
    void addContact(user_id owner_id, user_id contact_id, const std::string& contact_name);
    void removeContact(user_id owner_id, user_id contact_id);
    std::vector<Contact> getContacts(user_id user_id) const;
    std::vector<Contact> searchContacts(user_id user_id, const std::string& query) const;
    
    ContactManager& operator+(const Contact& contact);
    ContactManager& operator-(const Contact& contact);
    ContactManager& operator+(const std::vector<Contact>& contacts_list);
    ContactManager& operator-(const std::vector<Contact>& contacts_list);
};

#endif