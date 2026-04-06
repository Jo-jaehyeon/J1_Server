#include "pch.h"
#include "ChatPacketHandler.h"
#include "ChatSession.h"
#include "ChatMember.h"
#include "Utils/ObjectUtils.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& len)
{
    return false;
}

bool Handle_REQ_ENTER_ROOM(SessionPtr& session, Chat::REQ_ENTER_ROOM& pkt)
{
	ChatMemberPtr player = ObjectUtils::CreatePlayer(pkt.name());

	ChatSessionPtr cs = static_pointer_cast<ChatSession>(session);
	player->session = cs;
	cs->player.store(player);

	GRoom->HandleEnterPlayerLocked(player);

	return true;
}

bool Handle_REQ_LEAVE_ROOM(SessionPtr& session, Chat::REQ_LEAVE_ROOM& pkt)
{
	ChatSessionPtr cs = static_pointer_cast<ChatSession>(session);
	cs->player.store({});

	GRoom->HandleLeavePlayerLocked(pkt.player_id());

	return true;
}

bool Handle_REQ_CHAT(SessionPtr& session, Chat::REQ_CHAT& pkt)
{
	std::string content = pkt.message();
	spdlog::trace("Receive Chat Request {}", content);
	
	GRoom->Broadcast(pkt);

    return true;
}