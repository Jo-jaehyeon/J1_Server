#include "pch.h"
#include "AuctionPacketHandler.h"
#include "AuctionSession.h"
#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "SqlUtils.h"

SS_AuctionHandlerFunc GSS_AuctionPacketHandler[UINT16_MAX];

//-------------------------
//		Auction Server
//-------------------------
bool Handle_SS_Auction_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
{
	return false;
}

bool Handle_REQ_AUCTION_LIST(SessionPtr& session, SS_Auction::REQ_AUCTION_LIST& pkt)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	string listQuery = pkt.mylist() ? 
		"SELECT * FROM auction_listings WHERE expired_at > NOW() AND seller_id = ?  ORDER BY list_id ASC" :
		"SELECT * FROM auction_listings WHERE expired_at > NOW() ORDER BY list_id ASC LIMIT 100";

	auto list_result = pkt.mylist() ? SqlUtils::executeQuery(conn->sql_connection, "J1_DB", listQuery, pkt.player_id()) :
		SqlUtils::executeQuery(conn->sql_connection, "J1_DB", listQuery);

	SS_Auction::RES_AUCTION_LIST listPkt;
	listPkt.set_session_id(pkt.session_id());
	listPkt.set_mylist(pkt.mylist());

	while (list_result->next())
	{
		SS_Auction::AuctionItemInfo* temp = listPkt.add_listinfo();
		temp->set_list_id(list_result->getInt("list_id"));
		temp->set_player_id(list_result->getInt("seller_id"));
		temp->set_item_id(list_result->getInt("item_id"));
		temp->set_count(list_result->getInt("count"));
		temp->set_price(list_result->getInt("price"));
		temp->set_expired_at(list_result->getString("expired_at"));
	}

	if (AuctionSessionPtr _Session = static_pointer_cast<AuctionSession>(session))
		_Session->SendPacket(listPkt, SS_Auction::PacketType::PKT_RES_AUCTION_LIST);

	GConnectionPool->unborrow(conn);

	return true;
}

bool Handle_REQ_RECEIPT_LIST(SessionPtr& session, SS_Auction::REQ_RECEIPT_LIST& pkt)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	string listQuery = "SELECT * FROM auction_receipts WHERE owner_player_id = ? ORDER BY list_id ASC LIMIT 100";

	auto list_result = SqlUtils::executeQuery(conn->sql_connection, "J1_DB", listQuery, pkt.player_id());

	SS_Auction::RES_RECEIPT_LIST listPkt;
	listPkt.set_session_id(pkt.session_id());

	while (list_result->next())
	{
		SS_Auction::AuctionItemInfo* temp = listPkt.add_receiptlist();
		temp->set_list_id(list_result->getInt("receipt_id"));
		temp->set_receipt_type(list_result->getInt("receipt_type"));
		temp->set_item_id(list_result->getInt("item_id"));
		temp->set_count(list_result->getInt("quantity"));
		temp->set_price(list_result->getInt("gold_amount"));
		
	}

	if (AuctionSessionPtr _Session = static_pointer_cast<AuctionSession>(session))
		_Session->SendPacket(listPkt, SS_Auction::PacketType::PKT_RES_RECEIPT_LIST);

	GConnectionPool->unborrow(conn);

	return true;
}

bool Handle_REQ_REGIST_ITEM(SessionPtr& session, SS_Auction::REQ_REGIST_ITEM& pkt)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	string registQuery = "INSERT INTO auction_listings (seller_id, item_id, remaining_quantity, price, expired_at) "
		"VALUES(?, ?, ?, ?, NOW() + INTERVAL ? HOUR)";

	SS_Auction::AuctionItemInfo Info = pkt.registinfo(0);
	int expiredTime = (Info.expired_at() == "24") ? 24 : 48;
	int list_result = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", registQuery, Info.player_id(), Info.item_id(), Info.count(), Info.price(), expiredTime);

	SS_Auction::RES_REGIST_ITEM registPkt;
	registPkt.set_result((list_result > 0));
	registPkt.set_session_id(pkt.session_id());

	if (AuctionSessionPtr _Session = static_pointer_cast<AuctionSession>(session))
		_Session->SendPacket(registPkt, SS_Auction::PacketType::PKT_RES_REGIST_ITEM);

	GConnectionPool->unborrow(conn);

	return false;
}

bool Handle_REQ_PURCHASE_ITEM(SessionPtr& session, SS_Auction::REQ_PURCHASE_ITEM& pkt)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	bool result = true;
	int64 totalPrice = 0;
	auto info = pkt.purchaseinfo(0);
	try {
		// 오토커밋 끄기 -> 트랜잭션 시작
		conn->SetAutoCommit(false);

		// 매물 확인
		if (info.count() <= 0)            throw sql::SQLException("invalid count");

		string checkQuery = "SELECT remaining_quantity, price, seller_id, item_id "
			"FROM auction_listings "
			"WHERE listing_id = ? AND expire_at > NOW() "
			"FOR UPDATE";
		auto check_result = SqlUtils::executeQuery(conn->sql_connection, "J1_DB", checkQuery, info.list_id());

		if (!check_result->next())
			throw sql::SQLException("listing not found or expired");

		int32 remaining = check_result->getInt("remaining_quantity");
		int64 unitPrice = check_result->getInt64("price");
		int64 sellerId = check_result->getInt64("seller_id");
		int32 ItemId = check_result->getInt("item_id");

		totalPrice = unitPrice * info.count();
		if (sellerId == info.player_id()) throw sql::SQLException("own listing");

		// TODO 개수 확인해서 바로 Rollback하게
		
		// 구매
		//   골드 차감
		string purchaseQuery1 = "UPDATE character_base SET gold = gold - ? WHERE account_id = ? AND gold >= ?";
		if(SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", purchaseQuery1, totalPrice, info.player_id(), totalPrice) == 0)
			throw sql::SQLException("not enough gold");
		
		//   수량 차감
		int puchaseresult;
		if (remaining == info.count())
			puchaseresult = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", "DELETE FROM auction_listings WHERE listing_id = ?", info.list_id());
		else
		{
			string purchaseQuery2 = "UPDATE auction_listings SET remaining_quantity = remaining_quantity - ? WHERE listing_id = ? AND remaining_quantity > ?";
			puchaseresult = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", purchaseQuery2, info.count(), info.list_id(), info.count());
		}
		if(puchaseresult == 0)	throw sql::SQLException("quantity update failed");


		// 거래 기록 
		string transactionQuery = "INSERT INTO auction_transactions (item_id, quantity, total_price) VALUES(?, ?, ?)";
		if (SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", transactionQuery, ItemId, info.count(), totalPrice) == 0)
			throw sql::SQLException("transaction update failed");

		string receiptQuery = "INSERT INTO auction_receipts (owner_player_id, receipt_type, gold_amount, item_id, quantity, source_listing_id) "
			"VALUE(?, 3, NULL, ?, ?, ?), "		// Buyer
			"(?, 1, ?, NULL, NULL, ?)";	// Seller
		if (SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", receiptQuery, info.player_id(), ItemId, info.count(), info.list_id(), sellerId, totalPrice, info.list_id()) != 2)
			throw sql::SQLException("receipt update failed");

		conn->Commit();
	}
	catch (const sql::SQLException& e) {
		// 실패 시 롤백
		conn->Rollback();
		spdlog::error("player_id : {}  {}번 아이템 구매 실패, 롤백: {}", info.player_id(), info.list_id(), e.what());
		result = false;
	}

	// 오토커밋 복구
	conn->SetAutoCommit(true);
	GConnectionPool->unborrow(conn);


	// 결과 패킷 전송
	SS_Auction::RES_PURCHASE_ITEM purchasePkt;
	purchasePkt.set_result(result);
	purchasePkt.set_session_id(pkt.session_id());
	purchasePkt.set_gold(totalPrice);

	if (AuctionSessionPtr _Session = static_pointer_cast<AuctionSession>(session))
		_Session->SendPacket(purchasePkt, SS_Auction::PacketType::PKT_RES_PURCHASE_ITEM);

	return true;
}

bool Handle_REQ_RECEIPT_ITEM(SessionPtr& session, SS_Auction::REQ_RECEIPT_ITEM& pkt)
{
	return true;
}