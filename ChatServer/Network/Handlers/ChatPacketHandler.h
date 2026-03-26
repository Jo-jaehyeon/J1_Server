#pragma once
#include "../Protocol/ChatProtocol.pb.h"
#include "Packet.h"

using PacketHandlerFunc = std::function<bool(SessionPtr&, boost::asio::mutable_buffer&, int32&)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum
{
PKT_REQ_LOGIN = 1000,
PKT_RES_LOGIN = 1001,
PKT_REQ_LIST = 1002,
PKT_RES_LIST = 1003,
PKT_REQ_CHAT = 1004,
PKT_RES_CHAT = 1005,
PKT_REQ_CHATNOTI = 1006,
PKT_RES_CHATNOTI = 1007,
};

// Custom Handler
bool Handle_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset);
bool Handle_REQ_LOGIN(SessionPtr& session, Chat::REQ_LOGIN& pkt);
bool Handle_REQ_LIST(SessionPtr& session, Chat::REQ_LIST& pkt);
bool Handle_REQ_CHAT(SessionPtr& session, Chat::REQ_CHAT& pkt);
bool Handle_REQ_CHATNOTI(SessionPtr& session, Chat::REQ_CHATNOTI& pkt);


class ChatPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_REQ_LOGIN] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_LOGIN>(Handle_REQ_LOGIN, session, buffer, offset);
			};
		GPacketHandler[PKT_REQ_LIST] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_LIST>(Handle_REQ_LIST, session, buffer, offset);
			};
		GPacketHandler[PKT_REQ_CHAT] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_CHAT>(Handle_REQ_CHAT, session, buffer, offset);
			};
		GPacketHandler[PKT_REQ_CHATNOTI] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_CHATNOTI>(Handle_REQ_CHATNOTI, session, buffer, offset);
			};
	}

	static bool HandlePacket(SessionPtr& session, const PacketHeader& header, char* ptr, size_t size)
	{
		boost::asio::mutable_buffer buffer = boost::asio::buffer(ptr, size);
		int offset = 4;

		return GPacketHandler[header.Code](session, buffer, offset);
	}

private:
	template<typename PacketType, typename ProcessFunc>
	static bool DispatchPacket(ProcessFunc func, SessionPtr & session, boost::asio::mutable_buffer & buffer, int32 & offset)
	{
		PacketType pkt;
		if (!PacketUtil::Parse(pkt, buffer, buffer.size(), offset))
		{
			spdlog::error("Failed to Handle Packet");
			return false;
		}

		return func(session, pkt);
	}
};