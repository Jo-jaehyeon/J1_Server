#include "pch.h"
#include "ChatRoom.h"
#include "ChatMember.h"
#include "ChatSession.h"

ChatRoomPtr GRoom = std::make_shared<ChatRoom>();

bool ChatRoom::EnterPlayer(ChatMemberPtr player)
{
	// 있다면 문제가 있다
	if (_members.find(player->playerInfo->player_id()) != _members.end())
		return false;

	_members.insert(make_pair(player->playerInfo->player_id(), player));

	player->room.store(shared_from_this());

	return true;
}

bool ChatRoom::LeavePlayer(int32 player_id)
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
	Chat::RES_LEAVE_ROOM leavePkt;
	leavePkt.set_result(success);
	if (auto session = player->session.lock())
		session->SendPacket(leavePkt, Chat::PacketType::PKT_RES_LEAVE_ROOM);


	if (success)
	{
		// player의 sesion 참조해제
		player->session.reset();

		// room에서 player 제거
		_members.erase(player_id);
	}

	return success;
}


bool ChatRoom::HandleEnterPlayerLocked(ChatMemberPtr player)
{
	//WRITE_LOCK;

	// TODO playerid 부여 
	player->playerInfo->set_player_id(_members.size()+1);
	bool success = EnterPlayer(player);

	Chat::RES_ENTER_ROOM enterPkt;
	enterPkt.set_result(success);
	if (auto session = player->session.lock())
		session->SendPacket(enterPkt, Chat::PacketType::PKT_RES_ENTER_ROOM);

	return success;
}

bool ChatRoom::HandleLeavePlayerLocked(int32 player_id)
{
	//WRITE_LOCK;

	bool success = LeavePlayer(player_id);

	return success;
}

void ChatRoom::Broadcast(google::protobuf::Message& pkt)
{
	for (auto& m : _members)
	{
		ChatMemberPtr player = m.second;

		if (auto session = player->session.lock())
			session->SendPacket(pkt, Chat::PacketType::PKT_RES_CHAT);
	}
}