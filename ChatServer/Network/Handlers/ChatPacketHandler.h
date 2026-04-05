#pragma once
#include "../Protocol/ChatProtocol.pb.h"
#include "Packet.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "J1.h"
#endif

using PacketHandlerFunc = std::function<bool(SessionPtr&, boost::asio::mutable_buffer&, int32&)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

/*
enum : uint16
{
	PKT_REQ_ENTER_ROOM = 1000,
	PKT_RES_ENTER_ROOM = 1001,
	PKT_REQ_CHAT = 1002,
	PKT_RES_CHAT = 1003,
};
*/

// Custom Handler
bool Handle_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset);
bool Handle_REQ_ENTER_ROOM(SessionPtr& session, Chat::REQ_ENTER_ROOM& pkt);
bool Handle_REQ_CHAT(SessionPtr& session, Chat::REQ_CHAT& pkt);


class ChatPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[Chat::MessageCode::PKT_REQ_ENTER_ROOM] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_ENTER_ROOM>(Handle_REQ_ENTER_ROOM, session, buffer, offset);
			};
		GPacketHandler[Chat::MessageCode::PKT_REQ_CHAT] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_CHAT>(Handle_REQ_CHAT, session, buffer, offset);
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
			
			#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
				UE_LOG(LogTemp, Warning, TEXT("Failed to Handle Packet"))
			#else
				spdlog::error("Failed to Handle Packet");
			#endif
			return false;
		}

		return func(session, pkt);
	}
};