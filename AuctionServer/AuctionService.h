#pragma once

namespace active911 { class MySQLConnection; }

class AuctionService
{
public:
	using Conn = active911::MySQLConnection;

	static void ProcessAuctionList(Conn& conn, const SessionPtr& session, const SS_Auction::REQ_AUCTION_LIST& pkt);
	static void ProcessReceiptList(Conn& conn, const SessionPtr& session, const SS_Auction::REQ_RECEIPT_LIST& pkt);
	static void ProcessRegistItem(Conn& conn, const SessionPtr& session, const SS_Auction::REQ_REGIST_ITEM& pkt);
	static void ProcessPurchaseItem(Conn& conn, const SessionPtr& session, const SS_Auction::REQ_PURCHASE_ITEM& pkt);

private:
	AuctionService() = delete;                    // static 함수만 모은 클래스

	using ItemList = google::protobuf::RepeatedPtrField<Game::AuctionItemInfo>;

	struct PurchaseResult
	{
		bool  success = false;
		int64 totalPrice = 0;     // 기존 동작 유지: 실패해도 계산까지 진행된 금액이 들어갈 수 있다
	};

	static void GetListings(Conn& conn, bool mine, int32 playerId, ItemList& out);
	static void GetReceipts(Conn& conn, uint64 playerId, ItemList& out);
	static bool Regist(Conn& conn, const Game::AuctionItemInfo& info);
	static PurchaseResult Purchase(Conn& conn, const Game::AuctionItemInfo& info);				 // 롤백 + 데드락 재시도 포함

	static void TryPurchase(Conn& conn, const Game::AuctionItemInfo& info, int64& totalPrice);   // 1회 시도, 실패하면 throw
	static void SafeRollback(Conn& conn);
	static void SafeRestoreAutoCommit(Conn& conn);

	// ---- 응답 ----
	static void ReplyPacket(const SessionPtr& session, google::protobuf::Message& msg, const short packetCode);
};

