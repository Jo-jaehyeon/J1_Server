#include "pch.h"
#include "AuctionPacketHandler.h"
#include "AuctionSession.h"
#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "SqlUtils.h"
#include "AuctionWorkerPool.h"
#include "AuctionService.h"

SS_AuctionHandlerFunc GSS_AuctionPacketHandler[UINT16_MAX];

namespace
{
	using Conn = AuctionWorkerPool::Conn;

	template<typename Pkt>
	std::shared_ptr<Pkt> TakeOwnership(Pkt& pkt)
	{
		auto owned = std::make_shared<Pkt>();
		owned->Swap(&pkt);
		return owned;
	}
}

//-------------------------
//		Auction Server
//-------------------------
bool Handle_SS_Auction_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
{
	return false;
}

bool Handle_REQ_AUCTION_LIST(SessionPtr& session, SS_Auction::REQ_AUCTION_LIST& pkt)
{
	auto req = TakeOwnership(pkt);

	AuctionWorkerPool::Instance().PostRead([session, req](Conn& conn)
		{
			AuctionService::ProcessAuctionList(conn, session, *req);
		});

	return true;
}

bool Handle_REQ_RECEIPT_LIST(SessionPtr& session, SS_Auction::REQ_RECEIPT_LIST& pkt)
{
	auto req = TakeOwnership(pkt);

	AuctionWorkerPool::Instance().PostRead([session, req](Conn& conn)
		{
			AuctionService::ProcessReceiptList(conn, session, *req);
		});

	return true;
}

bool Handle_REQ_REGIST_ITEM(SessionPtr& session, SS_Auction::REQ_REGIST_ITEM& pkt)
{
	if (pkt.registinfo_size() == 0)
		return false;

	auto req = TakeOwnership(pkt);
	const uint64 key = static_cast<uint64>(req->registinfo(0).player_id());     // 같은 판매자의 등록은 한 워커에서 순서대로

	AuctionWorkerPool::Instance().PostWrite(key, [session, req](Conn& conn)
		{
			AuctionService::ProcessRegistItem(conn, session, *req);
		});

	return true;
}

bool Handle_REQ_PURCHASE_ITEM(SessionPtr& session, SS_Auction::REQ_PURCHASE_ITEM& pkt)
{
	if (pkt.purchaseinfo_size() == 0)
		return false;

	auto req = TakeOwnership(pkt);
	const uint64 key = static_cast<uint64>(req->purchaseinfo(0).list_id());     // 같은 매물의 구매는 한 워커에서 순서대로

	AuctionWorkerPool::Instance().PostWrite(key, [session, req](Conn& conn)
		{
			AuctionService::ProcessPurchaseItem(conn, session, *req);
		});

	return true;
}

bool Handle_REQ_RECEIPT_ITEM(SessionPtr& session, SS_Auction::REQ_RECEIPT_ITEM& pkt)
{
	return true;
}