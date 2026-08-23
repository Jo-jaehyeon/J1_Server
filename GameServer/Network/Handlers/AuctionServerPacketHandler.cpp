#include "pch.h"
#include "AuctionServerPacketHandler.h"

SS_AuctionHandlerFunc GSS_AuctionPacketHandler[UINT16_MAX];

// 경매장 서버와 통신을 위한 패킷을 전담
bool Handle_AuctionServer_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
{
	return true;
}

bool Handle_RES_AUCTION_LIST(SessionPtr& session, SS_Auction::RES_AUCTION_LIST& pkt)
{
	return true;
}