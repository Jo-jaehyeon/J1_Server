#include "pch.h"
#include "AuctionPacketHandler.h"
#include "AuctionSession.h"
#include "Network/Protocol/SS_AuctionProtocol.pb.h"

//-------------------------
//		Game Server
//-------------------------

bool Handle_REQ_AUCTION_LIST(SessionPtr& session, Game::REQ_AUCTION_LIST& pkt)
{
	SS_Auction::REQ_AUCTION_LIST listPkt;
	listPkt.set_session_id(session->GetSessionId());
	listPkt.set_player_id(pkt.player_id());
	listPkt.set_search_item(pkt.search_item());
	listPkt.set_mylist(pkt.mylist());
	
	if (GAuctionSession)
		GAuctionSession->SendPacket(listPkt, SS_Auction::PacketType::PKT_REQ_AUCTION_LIST);

	return true;
}

bool Handle_REQ_RECEIPT_LIST(SessionPtr& session, Game::REQ_RECEIPT_LIST& pkt)
{
	SS_Auction::REQ_RECEIPT_LIST listPkt;
	listPkt.set_session_id(session->GetSessionId());
	listPkt.set_player_id(pkt.player_id());

	if (GAuctionSession)
		GAuctionSession->SendPacket(listPkt, SS_Auction::PacketType::PKT_REQ_RECEIPT_LIST);

	return true;
}

bool Handle_REQ_REGIST_ITEM(SessionPtr& session, Game::REQ_REGIST_ITEM& pkt)
{
	// 등록 요청 정보는 한개만 넘어옴
	Game::AuctionItemInfo info = pkt.registinfo(0);

	SS_Auction::REQ_REGIST_ITEM registPkt;
	registPkt.set_session_id(session->GetSessionId());

	Game::AuctionItemInfo* temp = registPkt.add_registinfo();
	temp->set_player_id(info.player_id());
	temp->set_item_id(info.item_id());
	temp->set_count(info.count());
	temp->set_price(info.price());
	temp->set_expired_at(info.expired_at());

	if (GAuctionSession)
		GAuctionSession->SendPacket(registPkt, SS_Auction::PacketType::PKT_REQ_REGIST_ITEM);

	return true;
}

bool Handle_REQ_PURCHASE_ITEM(SessionPtr& session, Game::REQ_PURCHASE_ITEM& pkt)
{
	// 구매 요청 정보는 한개만 넘어옴
	SS_Auction::REQ_PURCHASE_ITEM purchasePkt;
	purchasePkt.set_session_id(session->GetSessionId());
	*purchasePkt.add_purchaseinfo() = pkt.purchaseinfo(0);

	if (GAuctionSession)
		GAuctionSession->SendPacket(purchasePkt, SS_Auction::PacketType::PKT_REQ_PURCHASE_ITEM);

	return true;
}

bool Handle_REQ_RECEIPT_ITEM(SessionPtr& session, Game::REQ_RECEIPT_ITEM& pkt)
{
	SS_Auction::REQ_RECEIPT_ITEM receiptPkt;
	receiptPkt.set_session_id(session->GetSessionId());
	receiptPkt.set_player_id(pkt.player_id());
	receiptPkt.set_receipt_id(pkt.receipt_id());

	if (GAuctionSession)
		GAuctionSession->SendPacket(receiptPkt, SS_Auction::PacketType::PKT_REQ_RECEIPT_ITEM);

	return true;
}