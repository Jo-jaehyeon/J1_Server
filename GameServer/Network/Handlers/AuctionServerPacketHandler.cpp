#include "pch.h"
#include "AuctionServerPacketHandler.h"
#include "GamePacketHandler.h"
#include "GameSession.h"
#include "Utils/GameSessionManager.h"

SS_AuctionHandlerFunc GSS_AuctionPacketHandler[UINT16_MAX];

// 경매장 서버와 통신을 위한 패킷을 전담
bool Handle_SS_Auction_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
{
	return true;
}

bool Handle_RES_AUCTION_LIST(SessionPtr& session, SS_Auction::RES_AUCTION_LIST& pkt)
{
	Game::RES_AUCTION_LIST listPkt;
	listPkt.set_mylist(pkt.mylist());

	for (auto row : pkt.listinfo())
	{
		Game::AuctionItemInfo* temp = listPkt.add_itemlist();
		temp->set_list_id(row.list_id());
		temp->set_item_id(row.item_id());
		temp->set_count(row.count());
		temp->set_price(row.price());
		temp->set_expired_at(row.expired_at());
	}

	int session_id = pkt.session_id();
	
	if (GameSessionPtr _Session = GameSessionManager::Instance().Find(session_id))
		_Session->SendPacket(listPkt, Game::PacketType::PKT_RES_AUCTION_LIST);

	return true;
}

bool Handle_RES_RECEIPT_LIST(SessionPtr& session, SS_Auction::RES_RECEIPT_LIST& pkt)
{
	Game::RES_RECEIPT_LIST listPkt;
	
	for (auto row : pkt.receiptlist())
	{
		Game::AuctionItemInfo* temp = listPkt.add_receiptlist();
		temp->set_list_id(row.list_id());
		temp->set_receipt_type(row.receipt_type());
		temp->set_item_id(row.item_id());
		temp->set_count(row.count());
		temp->set_price(row.price());
	}

	int session_id = pkt.session_id();

	if (GameSessionPtr _Session = GameSessionManager::Instance().Find(session_id))
		_Session->SendPacket(listPkt, Game::PacketType::PKT_RES_RECEIPT_LIST);

	return true;
}

bool Handle_RES_REGIST_ITEM(SessionPtr& session, SS_Auction::RES_REGIST_ITEM& pkt)
{
	Game::RES_REGIST_ITEM registPkt;
	registPkt.set_result(pkt.result());

	int session_id = pkt.session_id();

	if (GameSessionPtr _Session = GameSessionManager::Instance().Find(session_id))
		_Session->SendPacket(registPkt, Game::PacketType::PKT_RES_REGIST_ITEM);

	return true;
}

bool Handle_RES_PURCHASE_ITEM(SessionPtr& session, SS_Auction::RES_PURCHASE_ITEM& pkt)
{
	Game::RES_PURCHASE_ITEM purchasePkt;
	purchasePkt.set_result(pkt.result());
	purchasePkt.set_gold(pkt.gold());

	int session_id = pkt.session_id();

	if (GameSessionPtr _Session = GameSessionManager::Instance().Find(session_id))
		_Session->SendPacket(purchasePkt, Game::PacketType::PKT_RES_REGIST_ITEM);

	return true;
}

bool Handle_RES_RECEIPT_ITEM(SessionPtr& session, SS_Auction::RES_RECEIPT_ITEM& pkt)
{
	return true;
}