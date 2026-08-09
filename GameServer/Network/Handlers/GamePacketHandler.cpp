#include "pch.h"
#include "GamePacketHandler.h"

GameHandlerFunc GGamePacketHandler[UINT16_MAX];

bool Handle_Game_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
{
	spdlog::warn("Invalid Packet Received");
	return false;
}