CREATE TABLE `users` (
	`user_id` INTEGER NOT NULL AUTO_INCREMENT,
	`username` VARCHAR(100) NOT NULL,
	`password_hash` VARCHAR(255) NOT NULL,
	`online` BOOLEAN NOT NULL DEFAULT false,
	`user_last_seen` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`user_created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(`user_id`)
);


CREATE TABLE `contacts` (
	`contact_row_id` INTEGER NOT NULL AUTO_INCREMENT,
	`owner_id` INTEGER NOT NULL,
	`contact_id` INTEGER NOT NULL,
	`contact_name` VARCHAR(100) NOT NULL,
	`contact_added_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(`contact_row_id`)
);


CREATE INDEX `idx_contacts_owner`
ON `contacts` (`owner_id`);
CREATE TABLE `blocked_users` (
	`user_id` INTEGER NOT NULL,
	`blocked_user_id` INTEGER NOT NULL,
	`blocked_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(`user_id`, `blocked_user_id`)
);


CREATE TABLE `chats` (
	`chat_id` INTEGER NOT NULL AUTO_INCREMENT,
	`type` TINYINT NOT NULL,
	`chat_created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(`chat_id`)
);


CREATE TABLE `chat_participants` (
	`chat_id` INTEGER NOT NULL,
	`user_id` INTEGER NOT NULL,
	`participant_role` TINYINT NOT NULL DEFAULT 2,
	`membership_status` TINYINT NOT NULL DEFAULT 0,
	`joined_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`left_at` DATETIME,
	`banned_at` DATETIME,
	PRIMARY KEY(`chat_id`, `user_id`)
);


CREATE INDEX `idx_chat_participants_user`
ON `chat_participants` (`user_id`);
CREATE TABLE `messages` (
	`message_id` INTEGER NOT NULL AUTO_INCREMENT,
	`sender_id` INTEGER NOT NULL,
	`chat_id` INTEGER NOT NULL,
	`content` VARCHAR(1000) NOT NULL,
	`message_created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`status` TINYINT NOT NULL DEFAULT 0,
	`type` TINYINT NOT NULL DEFAULT 0,
	PRIMARY KEY(`message_id`)
);


ALTER TABLE `contacts`
ADD FOREIGN KEY(`owner_id`) REFERENCES `users`(`user_id`)
ON UPDATE NO ACTION ON DELETE CASCADE;
ALTER TABLE `contacts`
ADD FOREIGN KEY(`contact_id`) REFERENCES `users`(`user_id`)
ON UPDATE NO ACTION ON DELETE CASCADE;
ALTER TABLE `blocked_users`
ADD FOREIGN KEY(`user_id`) REFERENCES `users`(`user_id`)
ON UPDATE NO ACTION ON DELETE CASCADE;
ALTER TABLE `blocked_users`
ADD FOREIGN KEY(`blocked_user_id`) REFERENCES `users`(`user_id`)
ON UPDATE NO ACTION ON DELETE CASCADE;
ALTER TABLE `chat_participants`
ADD FOREIGN KEY(`chat_id`) REFERENCES `chats`(`chat_id`)
ON UPDATE NO ACTION ON DELETE CASCADE;
ALTER TABLE `chat_participants`
ADD FOREIGN KEY(`user_id`) REFERENCES `users`(`user_id`)
ON UPDATE NO ACTION ON DELETE CASCADE;
ALTER TABLE `messages`
ADD FOREIGN KEY(`sender_id`) REFERENCES `users`(`user_id`)
ON UPDATE NO ACTION ON DELETE CASCADE;
ALTER TABLE `messages`
ADD FOREIGN KEY(`chat_id`) REFERENCES `chats`(`chat_id`)
ON UPDATE NO ACTION ON DELETE SET NULL;