#pragma once
#include "Packet.h"
#include "BasicPacketHandler.h"
#include "BattlePacketHandler.h"
#include "LobbyPacketHandler.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "J1.h"
#endif

using GameHandlerFunc = std::function<bool(SessionPtr&, boost::asio::mutable_buffer&, int32&)>;
extern GameHandlerFunc GGamePacketHandler[UINT16_MAX];

bool Handle_Game_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset);

class GamePacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GGamePacketHandler[i] = Handle_Game_INVALID;

		GGamePacketHandler[Game::PacketType::PKT_REQ_MOVE] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_MOVE>(Handle_REQ_MOVE, session, buffer, offset);
			};
		GGamePacketHandler[Game::PacketType::PKT_REQ_ATTACK] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_ATTACK>(Handle_REQ_ATTACK, session, buffer, offset);
			};
		GGamePacketHandler[Game::PacketType::PKT_REQ_CHECK_TOKENVALID] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_CHECK_TOKENVALID>(Handle_REQ_CHECK_TOKENVALID, session, buffer, offset);
			};
		GGamePacketHandler[Game::PacketType::PKT_REQ_CHARACTER_LIST] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_CHARACTER_LIST>(Handle_REQ_CHARACTER_LIST, session, buffer, offset);
			};
		GGamePacketHandler[Game::PacketType::PKT_REQ_CHECK_NICKNAME] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_CHECK_NICKNAME>(Handle_REQ_CHECK_NICKNAME, session, buffer, offset);
			};
		GGamePacketHandler[Game::PacketType::PKT_REQ_CREATE_CHARACTER] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_CREATE_CHARACTER>(Handle_REQ_CREATE_CHARACTER, session, buffer, offset);
			};
		GGamePacketHandler[Game::PacketType::PKT_REQ_DELETE_CHARACTER] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<Game::REQ_DELETE_CHARACTER>(Handle_REQ_DELETE_CHARACTER, session, buffer, offset);
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