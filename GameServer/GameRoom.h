#pragma once

#include "pch.h"

class GameRoom : public std::enable_shared_from_this<GameRoom>
{
private:
	bool EnterPlayer(GameMemberPtr player);
	bool LeavePlayer(int32 player_id);

public:
	bool HandleEnterPlayerLocked(GameMemberPtr player);
	bool HandleLeavePlayerLocked(int32 player_id);

	void Broadcast(google::protobuf::Message& pkt);

private:
	std::unordered_map<uint64, GameMemberPtr> _members;
};

extern GameRoomPtr GRoom;