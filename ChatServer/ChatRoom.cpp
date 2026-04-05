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

void ChatRoom::Broadcast(const std::string& msg)
{
	// TODO : Chatserver에선 msg전달이 주목적
	// string에 대한 chat 요청 패킷으로 감싸 보내게 하자
	Chat::RES_CHAT ChatPkt;
	ChatPkt.set_message(msg);

	for (auto& m : _members)
	{
		ChatMemberPtr player = m.second;

		if (auto session = player->session.lock())
			session->SendPacket(ChatPkt, Chat::MessageCode::PKT_RES_CHAT);
	}
}