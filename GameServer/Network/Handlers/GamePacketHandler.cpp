#include "pch.h"
#include "GamePacketHandler.h"

GameHandlerFunc GGamePacketHandler[UINT16_MAX];

bool Handle_Game_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
{
	return false;
}

bool Handle_REQ_CHARACTER_LIST(SessionPtr& session, Game::REQ_CHARACTER_LIST& pkt)
{
	return false;
}

bool Handle_REQ_ENTER_GAME(SessionPtr& session, Game::REQ_ENTER_GAME& pkt)
{
	return false;
}

bool Handle_REQ_LEAVE_GAME(SessionPtr& session, Game::REQ_LEAVE_GAME& pkt)
{
	return false;
}
