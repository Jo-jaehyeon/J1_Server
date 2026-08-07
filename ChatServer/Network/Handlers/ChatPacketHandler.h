#pragma once
#include "../Protocol/ChatProtocol.pb.h"
#include "Packet.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "J1.h"
#endif

using ChatHandlerFunc = std::function<bool(SessionPtr&, boost::asio::mutable_buffer&, int32&)>;
extern ChatHandlerFunc GChatPacketHandler[UINT16_MAX];

// Custom Handler
bool Handle_Chat_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset);
bool Handle_REQ_ENTER_CHATROOM(SessionPtr& session, Chat::REQ_ENTER_CHATROOM&pkt);
bool Handle_REQ_LEAVE_CHATROOM(SessionPtr& session, Chat::REQ_LEAVE_CHATROOM&pkt);
bool Handle_REQ_CHAT(SessionPtr& session, Chat::REQ_CHAT&pkt);

class ChatPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GChatPacketHandler[i] = Handle_Chat_INVALID;
		GChatPacketHandler[Chat::PacketType::PKT_REQ_ENTER_CHATROOM] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_ENTER_CHATROOM>(Handle_REQ_ENTER_CHATROOM, session, buffer, offset);
			};
		GChatPacketHandler[Chat::PacketType::PKT_REQ_LEAVE_CHATROOM] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_LEAVE_CHATROOM>(Handle_REQ_LEAVE_CHATROOM, session, buffer, offset);
			};
		GChatPacketHandler[Chat::PacketType::PKT_REQ_CHAT] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Chat::REQ_CHAT>(Handle_REQ_CHAT, session, buffer, offset);
			};
	}

	static bool HandlePacket(SessionPtr& session, const PacketHeader& header, char* ptr, size_t size)
	{
		boost::asio::mutable_buffer buffer = boost::asio::buffer(ptr, size);
		int offset = 4;

		return GChatPacketHandler[header.Code](session, buffer, offset);
	}

private:
	template<typename PacketType, typename ProcessFunc>
	static bool DispatchPacket(ProcessFunc func, SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
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