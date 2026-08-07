#pragma once
#include "../Protocol/GameProtocol.pb.h"
#include "Packet.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "J1.h"
#endif

using GameHandlerFunc = std::function<bool(SessionPtr&, boost::asio::mutable_buffer&, int32&)>;
extern GameHandlerFunc GGamePacketHandler[UINT16_MAX];

// Custom Handler
bool Handle_Game_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset);
bool Handle_REQ_CHARACTER_LIST(SessionPtr& session, Game::REQ_CHARACTER_LIST&pkt);
bool Handle_REQ_ENTER_GAME(SessionPtr& session, Game::REQ_ENTER_GAME&pkt);
bool Handle_REQ_LEAVE_GAME(SessionPtr& session, Game::REQ_LEAVE_GAME&pkt);

class GamePacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GGamePacketHandler[i] = Handle_Game_INVALID;
		GGamePacketHandler[Game::PacketType::PKT_REQ_CHARACTER_LIST] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_CHARACTER_LIST>(Handle_REQ_CHARACTER_LIST, session, buffer, offset);
			};
		GGamePacketHandler[Game::PacketType::PKT_REQ_ENTER_GAME] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_ENTER_GAME>(Handle_REQ_ENTER_GAME, session, buffer, offset);
			};
		GGamePacketHandler[Game::PacketType::PKT_REQ_LEAVE_GAME] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_LEAVE_GAME>(Handle_REQ_LEAVE_GAME, session, buffer, offset);
			};
	}

	static bool HandlePacket(SessionPtr& session, const PacketHeader& header, char* ptr, size_t size)
	{
		boost::asio::mutable_buffer buffer = boost::asio::buffer(ptr, size);
		int offset = 4;

		return GGamePacketHandler[header.Code](session, buffer, offset);
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