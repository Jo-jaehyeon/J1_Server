#include "pch.h"
#include "AuctionPacketHandler.h"
#include "AuctionSession.h"

SS_AuctionHandlerFunc GSS_AuctionPacketHandler[UINT16_MAX];

bool Handle_SS_Auction_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
{
	return false;
}

bool Handle_REQ_AUCTION_LIST(SessionPtr& session, SS_Auction::REQ_AUCTION_LIST& pkt)
{
	return false;
}
