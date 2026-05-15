#SET NAMES utf8mb4;

CREATE TABLE `users` (
	`user_id` INT NOT NULL AUTO_INCREMENT,
	`username` VARCHAR(100) NOT NULL,
	`password_hash` VARCHAR(255) NOT NULL,
	`online` TINYINT(1) NOT NULL DEFAULT FALSE,
	`user_last_seen` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`user_created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Общая таблица для контактов и заблокированных пользователей
CREATE TABLE `user_interconnect` (
	`user_id` INT NOT NULL COMMENT 'Пользователь, которому принадлежит связь',
	`connected_user_id` INT NOT NULL COMMENT 'Связанный пользователь (контакт или заблокированный)',
	`connected_user_name` VARCHAR(100) NOT NULL COMMENT 'Имя контакта (только для контактов, для блокировок можно заполнять как connected_user_id или оставить пустым)',
	`type` TINYINT NOT NULL COMMENT 'Тип связи: 1 - контакт, 2 - заблокированный',
	`connected_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Время добавления в контакты или блокировки',
	PRIMARY KEY (`user_id`, `connected_user_id`, `type`),
	KEY `idx_interconnect_user` (`user_id`),
	KEY `idx_interconnect_connected` (`connected_user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `chats` (
	`chat_id` INT NOT NULL AUTO_INCREMENT,
	`type` TINYINT NOT NULL,
	`chat_created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY (`chat_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `chat_participants` (
	`chat_id` INT NOT NULL,
	`user_id` INT NOT NULL,
	`participant_role` TINYINT NOT NULL DEFAULT 2,
	`membership_status` TINYINT NOT NULL DEFAULT 0,
	`joined_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`left_at` DATETIME NULL DEFAULT NULL,
	`banned_at` DATETIME NULL DEFAULT NULL,
	PRIMARY KEY (`chat_id`, `user_id`),
	KEY `idx_chat_participants_user` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `messages` (
	`message_id` INT NOT NULL AUTO_INCREMENT,
	`sender_id` INT NOT NULL,
	`chat_id` INT NULL,
	`content` VARCHAR(1000) NOT NULL,
	`message_created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`status` TINYINT NOT NULL DEFAULT 0,
	`type` TINYINT NOT NULL DEFAULT 0,
	PRIMARY KEY (`message_id`),
	KEY `idx_messages_chat` (`chat_id`),
	KEY `idx_messages_sender` (`sender_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Внешние ключи для таблицы user_interconnect
ALTER TABLE `user_interconnect`
	ADD CONSTRAINT `fk_interconnect_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`user_id`)
		ON DELETE CASCADE ON UPDATE RESTRICT,
	ADD CONSTRAINT `fk_interconnect_connected_user` FOREIGN KEY (`connected_user_id`) REFERENCES `users` (`user_id`)
		ON DELETE CASCADE ON UPDATE RESTRICT;

-- Внешние ключи для таблицы chat_participants
ALTER TABLE `chat_participants`
	ADD CONSTRAINT `fk_chat_participants_chat` FOREIGN KEY (`chat_id`) REFERENCES `chats` (`chat_id`)
		ON DELETE CASCADE ON UPDATE RESTRICT,
	ADD CONSTRAINT `fk_chat_participants_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`user_id`)
		ON DELETE CASCADE ON UPDATE RESTRICT;

-- Внешние ключи для таблицы messages
ALTER TABLE `messages`
	ADD CONSTRAINT `fk_messages_sender` FOREIGN KEY (`sender_id`) REFERENCES `users` (`user_id`)
		ON DELETE CASCADE ON UPDATE RESTRICT,
	ADD CONSTRAINT `fk_messages_chat` FOREIGN KEY (`chat_id`) REFERENCES `chats` (`chat_id`)
		ON DELETE SET NULL ON UPDATE RESTRICT;