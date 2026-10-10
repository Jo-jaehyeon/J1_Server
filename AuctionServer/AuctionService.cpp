#include "pch.h"
#include "AuctionService.h"

#include "AuctionSession.h"
#include "DB/MySQLConnection.h"
#include "SqlUtils.h"

namespace
{
	constexpr const char* kSchema = "J1_DB";

	constexpr int kMaxTxAttempts = 3;       // 데드락/락 타임아웃 시 트랜잭션 재시도 횟수(최초 포함)
	constexpr int kErrLockWaitTimeout = 1205;    // ER_LOCK_WAIT_TIMEOUT
	constexpr int kErrDeadlock = 1213;    // ER_LOCK_DEADLOCK
}

// ------------------------------------------------------------
//		요청 처리 (워커 스레드) : DB 처리 + 응답 전송
// ------------------------------------------------------------

void AuctionService::ReplyPacket(const SessionPtr& session, google::protobuf::Message& msg, const short packetCode)
{
	if (AuctionSessionPtr _Session = std::static_pointer_cast<AuctionSession>(session))
		_Session->SendPacket(msg, packetCode);
}

void AuctionService::ProcessAuctionList(Conn& conn, const SessionPtr& session, const SS_Auction::REQ_AUCTION_LIST& pkt)
{
	SS_Auction::RES_AUCTION_LIST res;
	res.set_session_id(pkt.session_id());
	res.set_request_id(pkt.request_id());
	res.set_mylist(pkt.mylist());

	GetListings(conn, pkt.mylist(), pkt.player_id(), *res.mutable_listinfo());

	ReplyPacket(session, res, SS_Auction::PacketType::PKT_RES_AUCTION_LIST);
}

void AuctionService::ProcessReceiptList(Conn& conn, const SessionPtr& session, const SS_Auction::REQ_RECEIPT_LIST& pkt)
{
	SS_Auction::RES_RECEIPT_LIST res;
	res.set_session_id(pkt.session_id());

	GetReceipts(conn, pkt.player_id(), *res.mutable_receiptlist());

	ReplyPacket(session, res, SS_Auction::PacketType::PKT_RES_RECEIPT_LIST);
}


void AuctionService::ProcessRegistItem(Conn& conn, const SessionPtr& session, const SS_Auction::REQ_REGIST_ITEM& pkt)
{
	const bool ok = Regist(conn, pkt.registinfo(0));

	SS_Auction::RES_REGIST_ITEM res;
	res.set_result(ok);
	res.set_session_id(pkt.session_id());

	ReplyPacket(session, res, SS_Auction::PacketType::PKT_RES_REGIST_ITEM);
}

void AuctionService::ProcessPurchaseItem(Conn& conn, const SessionPtr& session, const SS_Auction::REQ_PURCHASE_ITEM& pkt)
{
	const PurchaseResult purchase = Purchase(conn, pkt.purchaseinfo(0));

	SS_Auction::RES_PURCHASE_ITEM res;
	res.set_result(purchase.success);
	res.set_session_id(pkt.session_id());
	res.set_gold(purchase.totalPrice);

	ReplyPacket(session, res, SS_Auction::PacketType::PKT_RES_PURCHASE_ITEM);
}

// ---------------------------------
//               조회
// ---------------------------------
void AuctionService::GetListings(Conn& conn, bool mine, int32 playerId, ItemList& out)
{
	std::string listQuery = mine ?
		"SELECT * FROM auction_listings WHERE expired_at > NOW() AND seller_id = ?  ORDER BY listing_id ASC" :
		"SELECT * FROM auction_listings WHERE expired_at > NOW() ORDER BY listing_id ASC LIMIT 100";

	auto list_result = mine ? SqlUtils::executeQuery(conn.sql_connection, kSchema, listQuery, playerId) :
		SqlUtils::executeQuery(conn.sql_connection, kSchema, listQuery);

	while (list_result->next())
	{
		Game::AuctionItemInfo* temp = out.Add();
		temp->set_list_id(list_result->getInt("listing_id"));
		temp->set_player_id(list_result->getInt("seller_id"));
		temp->set_item_id(list_result->getInt("item_id"));
		temp->set_count(list_result->getInt("remaining_quantity"));
		temp->set_price(list_result->getInt("price"));
		temp->set_expired_at(list_result->getString("expired_at"));
	}
}

void AuctionService::GetReceipts(Conn& conn, uint64 playerId, ItemList& out)
{
	std::string listQuery = "SELECT * FROM auction_receipts WHERE owner_player_id = ? ORDER BY list_id ASC LIMIT 100";

	auto list_result = SqlUtils::executeQuery(conn.sql_connection, kSchema, listQuery, playerId);

	while (list_result->next())
	{
		Game::AuctionItemInfo* temp = out.Add();
		temp->set_list_id(list_result->getInt("receipt_id"));
		temp->set_receipt_type(list_result->getInt("receipt_type"));
		temp->set_item_id(list_result->getInt("item_id"));
		temp->set_count(list_result->getInt("quantity"));
		temp->set_price(list_result->getInt("gold_amount"));
	}
}

// ---------------------------------
//              등록
// ---------------------------------
bool AuctionService::Regist(Conn& conn, const Game::AuctionItemInfo& info)
{
	std::string registQuery = "INSERT INTO auction_listings (seller_id, item_id, remaining_quantity, price, expired_at) "
		"VALUES(?, ?, ?, ?, NOW() + INTERVAL ? HOUR)";

	int expiredTime = (info.expired_at() == "24") ? 24 : 48;
	int affected = SqlUtils::executeUpdate(conn.sql_connection, kSchema, registQuery, info.player_id(), info.item_id(), info.count(), info.price(), expiredTime);

	return affected > 0;
}

// ---------------------------------
//               구매
// ---------------------------------
AuctionService::PurchaseResult AuctionService::Purchase(Conn& conn, const Game::AuctionItemInfo& info)
{
	PurchaseResult result;

	for (int attempt = 1; attempt <= kMaxTxAttempts; ++attempt)
	{
		bool retry = false;
		int64 totalPrice = 0;

		try
		{
			TryPurchase(conn, info, totalPrice);
			result.success = true;
		}
		catch (const sql::SQLException& e)
		{
			// 실패 시 롤백
			SafeRollback(conn);
			result.success = false;

			// 동시에 여러 워커가 돌면 드물게 데드락/락 대기 타임아웃이 날 수 있다 -> 몇 번 재시도
			const int code = e.getErrorCode();
			if ((code == kErrDeadlock || code == kErrLockWaitTimeout) && attempt < kMaxTxAttempts)
			{
				retry = true;
				spdlog::warn("player_id : {}  {}번 아이템 구매 재시도 ({}/{}) : {}", info.player_id(), info.list_id(), attempt, kMaxTxAttempts, e.what());
			}
			else
			{
				spdlog::error("player_id : {}  {}번 아이템 구매 실패, 롤백: {}", info.player_id(), info.list_id(), e.what());
			}
		}

		result.totalPrice = totalPrice;

		// 오토커밋 복구
		SafeRestoreAutoCommit(conn);

		if (!retry)
			break;
	}

	return result;
}

// 트랜잭션 1회 시도. 어느 단계에서든 실패하면 sql::SQLException 을 던진다. (롤백은 호출자가 한다)
void AuctionService::TryPurchase(Conn& conn, const Game::AuctionItemInfo& info, int64& totalPrice)
{
	// 오토커밋 끄기 -> 트랜잭션 시작
	conn.SetAutoCommit(false);

	// 매물 확인
	if (info.count() <= 0)            throw sql::SQLException("invalid count");

	std::string checkQuery = "SELECT remaining_quantity, price, seller_id, item_id "
		"FROM auction_listings "
		"WHERE listing_id = ? AND expired_at > NOW() "
		"FOR UPDATE";
	auto check_result = SqlUtils::executeQuery(conn.sql_connection, kSchema, checkQuery, info.list_id());

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
	std::string purchaseQuery1 = "UPDATE character_base SET gold = gold - ? WHERE account_id = ? AND gold >= ?";
	if (SqlUtils::executeUpdate(conn.sql_connection, kSchema, purchaseQuery1, totalPrice, info.player_id(), totalPrice) == 0)
		throw sql::SQLException("not enough gold");

	//   수량 차감
	int puchaseresult;
	if (remaining == info.count())
		puchaseresult = SqlUtils::executeUpdate(conn.sql_connection, kSchema, "DELETE FROM auction_listings WHERE listing_id = ?", info.list_id());
	else
	{
		std::string purchaseQuery2 = "UPDATE auction_listings SET remaining_quantity = remaining_quantity - ? WHERE listing_id = ? AND remaining_quantity > ?";
		puchaseresult = SqlUtils::executeUpdate(conn.sql_connection, kSchema, purchaseQuery2, info.count(), info.list_id(), info.count());
	}
	if (puchaseresult == 0)	throw sql::SQLException("quantity update failed");


	// 거래 기록
	std::string transactionQuery = "INSERT INTO auction_transactions (item_id, quantity, total_price) VALUES(?, ?, ?)";
	if (SqlUtils::executeUpdate(conn.sql_connection, kSchema, transactionQuery, ItemId, info.count(), totalPrice) == 0)
		throw sql::SQLException("transaction update failed");

	std::string receiptQuery = "INSERT INTO auction_receipts (owner_player_id, receipt_type, gold_amount, item_id, quantity, source_listing_id) "
		"VALUE(?, 3, NULL, ?, ?, ?), "		// Buyer
		"(?, 1, ?, NULL, NULL, ?)";	// Seller
	if (SqlUtils::executeUpdate(conn.sql_connection, kSchema, receiptQuery, info.player_id(), ItemId, info.count(), info.list_id(), sellerId, totalPrice, info.list_id()) != 2)
		throw sql::SQLException("receipt update failed");

	conn.Commit();
}

void AuctionService::SafeRollback(Conn& conn)
{
	try { conn.Rollback(); }
	catch (const sql::SQLException& e) { spdlog::error("Rollback failed : {}", e.what()); }
}

void AuctionService::SafeRestoreAutoCommit(Conn& conn)
{
	try { conn.SetAutoCommit(true); }
	catch (const sql::SQLException& e) { spdlog::error("Restore autocommit failed : {}", e.what()); }
}