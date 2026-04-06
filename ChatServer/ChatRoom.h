#pragma once

#include "pch.h"

class ChatRoom : public std::enable_shared_from_this<ChatRoom>
{
private:
	bool EnterPlayer(ChatMemberPtr player);
	bool LeavePlayer(int32 player_id);

public:
	bool HandleEnterPlayerLocked(ChatMemberPtr player);
	bool HandleLeavePlayerLocked(int32 player_id);

	void Broadcast(google::protobuf::Message& pkt);

private:
	std::unordered_map<uint64, ChatMemberPtr> _members;
};

extern ChatRoomPtr GRoom;