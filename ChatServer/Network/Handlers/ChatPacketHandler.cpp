#include "pch.h"
#include "ChatPacketHandler.h"
#include "../../ChatMember.h"
#include "../../ChatSession.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& len)
{
    return false;
}

bool Handle_REQ_ENTER_ROOM(SessionPtr& session, Chat::REQ_ENTER_ROOM& pkt)
{
	// TODO
	ChatMemberPtr player = std::make_shared<ChatMember>();
	player->playerInfo->set_name(pkt.name());
	
	ChatSessionPtr cs = static_pointer_cast<ChatSession>(session);
	player->session = cs;
	cs->player.store(player);

	spdlog::info("Someone Enter chat Room");
	GRoom->HandleEnterPlayerLocked(player);

	return true;
}

bool Handle_REQ_CHAT(SessionPtr& session, Chat::REQ_CHAT& pkt)
{
	std::string content = pkt.message();
	spdlog::trace("Receive Chat Request {}", content);
	
	GRoom->Broadcast(pkt);

    return true;
}

/*
void HandleLoginReq(asio::mutable_buffer& buffer, const PacketHeader& header, int& offset)
{
	std::cout << "HandleLoginReq" << std::endl;
	Chat::REQ_LOGIN pbMsg;
	PacketUtil::Parse(pbMsg, buffer, header.Length, offset);
	std::cout << pbMsg.name(0) << std::endl;
	SetName(pbMsg.name(0));

	Chat::RES_LOGIN pbRes;
	pbRes.set_result(true);
	const size_t requiredSize = PacketUtil::RequiredSize(pbRes);

	// todo : delete buffer
	char* rawBuffer = new char[requiredSize];
	auto sendBuffer = asio::buffer(rawBuffer, requiredSize);
	PacketUtil::Serialize(sendBuffer, Chat::MessageCode::LOGIN_RES, pbRes);
	this->Send(sendBuffer);
}

void HandleListReq(asio::mutable_buffer& buffer, const PacketHeader& header, int& offset)
{
	std::cout << "HandleListReq" << std::endl;
	_room.SendList(this->shared_from_this());
}

void HandleChatReq(asio::mutable_buffer& buffer, const PacketHeader& header, int& offset)
{
	std::cout << "HandleChatReq" << std::endl;
	Chat::REQ_CHAT pbReq;
	PacketUtil::Parse(pbReq, buffer, header.Length, offset);

	Chat::REQ_CHATNOTI pbNoti;
	auto m = pbNoti.add_message();
	*m = pbReq.message(0);

	auto s = pbNoti.add_sender();
	*s = this->GetName();
	const size_t requiredSize = PacketUtil::RequiredSize(pbNoti);

	char* rawBuffer = new char[requiredSize];
	auto sendBuffer = asio::buffer(rawBuffer, requiredSize);
	PacketUtil::Serialize(sendBuffer, Chat::MessageCode::CHAT_NOTI_REQ, pbNoti);
	_room.Broadcast(sendBuffer);
}
*/