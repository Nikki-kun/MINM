#pragma once
#include <chrono>

enum class GroupType {
	DIALOG,
	GROUP,
	CHANNEL
};

enum class MessageStatus {
	SENT,
	DELIVERED,
	READ
};

enum class ParticipantStatus {
	ACTIVE,
	INACTIVE,
	BANNED,
	PENDING
};

using UserID = int;
using GroupID = int;
using MessageID = int;
using Timestamp = std::chrono::system_clock::time_point;