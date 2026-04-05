#pragma once

class ChatRoom : public std::enable_shared_from_this<ChatRoom>
{
private:
	bool EnterPlayer(ChatMemberPtr player);
	bool LeavePlayer(ChatMemberPtr player);

public:
	bool HandleEnterPlayerLocked(ChatMemberPtr player);
	bool HandleLeavePlayerLocked(ChatMemberPtr player);

	void Broadcast(const std::string& msg);

private:
	std::unordered_map<uint64, ChatMemberPtr> _members;
};

extern ChatRoomPtr GRoom;