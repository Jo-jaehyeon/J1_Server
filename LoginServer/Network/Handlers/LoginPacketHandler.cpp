#include "pch.h"
#include "LoginPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& len)
{
    return false;
}

bool Handle_REQ_LOGIN(SessionPtr& session, Login::REQ_LOGIN& pkt)
{
    return false;
}
