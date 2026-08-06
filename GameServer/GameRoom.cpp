#include "pch.h"
#include "GameRoom.h"
#include "GameMember.h"
#include "GameSession.h"

GameRoomPtr GRoom = std::make_shared<GameRoom>();

bool GameRoom::EnterPlayer(GameMemberPtr player)
{
	// 있다면 문제가 있다
	//if (_members.find(player->playerInfo->player_id()) != _members.end())
	//	return false;
	//
	//_members.insert(make_pair(player->playerInfo->player_id(), player));

	player->room.store(shared_from_this());

	return true;
}

bool GameRoom::LeavePlayer(int32 player_id)
{
	// 없다면 문제가 있다
	auto it = _members.find(player_id);
	if (it == _members.end())	return false;

	// player의 room 참조 해제
	auto player = it->second;
	player->room.store({});
	auto loaded = player->room.load();
	bool success = loaded.expired();

	// leave pkt 발송
	//Game::RES_LEAVE_ROOM leavePkt;
	//leavePkt.set_result(success);
	//if (auto session = player->session.lock())
	//	session->SendPacket(leavePkt, Chat::PacketType::PKT_RES_LEAVE_ROOM);


	if (success)
	{
		// player의 sesion 참조해제
		player->session.reset();

		// room에서 player 제거
		_members.erase(player_id);
	}

	return success;
}


bool GameRoom::HandleEnterPlayerLocked(GameMemberPtr player)
{
	//WRITE_LOCK;

	bool success = EnterPlayer(player);
	if(success)
		spdlog::info("Someone Enter chat Room");

	//Game::RES_ENTER_ROOM enterPkt;
	//enterPkt.set_player_id(player->playerInfo->player_id());
	//enterPkt.set_result(success);
	//if (auto session = player->session.lock())
	//	session->SendPacket(enterPkt, Game::PacketType::PKT_RES_ENTER_ROOM);

	return success;
}

bool GameRoom::HandleLeavePlayerLocked(int32 player_id)
{
	//WRITE_LOCK;

	bool success = LeavePlayer(player_id);
	if (success)
		spdlog::info("Someone Leave chat Room");

	return success;
}

void GameRoom::Broadcast(google::protobuf::Message& pkt)
{
	for (auto& m : _members)
	{
		GameMemberPtr player = m.second;

		//if (auto session = player->session.lock())
		//	session->SendPacket(pkt, Game::PacketType::PKT_RES_CHAT);
	}
}