#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include "../core/contact.h"
#include "../core/user.h"

class ContactManager {
private:
	std::unordered_map<UserID, std::vector<Contact>> user_contacts;
	std::unordered_map<UserID, std::vector<UserID>> contact_requests; // входящие запросы

public:
	// Управление контактами
	bool addContact(UserID owner_id, UserID contact_id, const std::string& alias = "");
	bool removeContact(UserID owner_id, UserID contact_id);
	bool updateContactAlias(UserID owner_id, UserID contact_id, const std::string& new_alias);

	// Запросы на добавление в контакты
	void sendContactRequest(UserID from_user, UserID to_user);
	bool acceptContactRequest(UserID user_id, UserID from_user);
	void declineContactRequest(UserID user_id, UserID from_user);

	// Получение контактов
	std::vector<Contact> getContacts(UserID user_id) const;
	std::vector<UserID> getContactRequests(UserID user_id) const;
	bool isContact(UserID user_id, UserID contact_id) const;
};