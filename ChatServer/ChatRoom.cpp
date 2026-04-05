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

bool ChatRoom::LeavePlayer(ChatMemberPtr player)
{
	// 없다면 문제가 있다
	if (_members.find(player->playerInfo->player_id()) == _members.end())
		return false;

	_members.erase(player->playerInfo->player_id());

	player->room.store({});
	auto loaded = player->room.load();

	return loaded.expired();
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
		session->SendPacket(enterPkt, Chat::MessageCode::PKT_RES_ENTER_ROOM);

	return success;
}

bool ChatRoom::HandleLeavePlayerLocked(ChatMemberPtr player)
{
	//WRITE_LOCK;

	bool success = LeavePlayer(player);

	return success;
}

void ChatRoom::Broadcast(google::protobuf::Message& pkt)
{
	for (auto& m : _members)
	{
		ChatMemberPtr player = m.second;

		if (auto session = player->session.lock())
			session->SendPacket(pkt, Chat::MessageCode::PKT_RES_CHAT);
	}
}