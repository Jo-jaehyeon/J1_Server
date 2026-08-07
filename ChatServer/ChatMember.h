#pragma once

class ChatSession;
class ChatRoom;

class ChatMember : public std::enable_shared_from_this<ChatMember>
{
public:
	ChatMember();
	virtual ~ChatMember();

public:
	Chat::ChatMemberInfo* playerInfo;
	std::weak_ptr<ChatSession> session;

public:
	std::atomic<std::weak_ptr<ChatRoom>> room;
};
