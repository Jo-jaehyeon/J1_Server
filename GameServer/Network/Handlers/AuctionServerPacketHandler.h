#pragma once
#include "../Protocol/SS_AuctionProtocol.pb.h"
#include "Packet.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "J1.h"
#endif

using SS_AuctionHandlerFunc = std::function<bool(SessionPtr&, boost::asio::mutable_buffer&, int32&)>;
extern SS_AuctionHandlerFunc GSS_AuctionPacketHandler[UINT16_MAX];

// Custom Handler
bool Handle_SS_Auction_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset);
bool Handle_RES_AUCTION_LIST(SessionPtr& session, SS_Auction::RES_AUCTION_LIST&pkt);
bool Handle_RES_RECEIPT_LIST(SessionPtr& session, SS_Auction::RES_RECEIPT_LIST&pkt);
bool Handle_RES_REGIST_ITEM(SessionPtr& session, SS_Auction::RES_REGIST_ITEM&pkt);
bool Handle_RES_PURCHASE_ITEM(SessionPtr& session, SS_Auction::RES_PURCHASE_ITEM&pkt);
bool Handle_RES_RECEIPT_ITEM(SessionPtr& session, SS_Auction::RES_RECEIPT_ITEM&pkt);

class AuctionServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GSS_AuctionPacketHandler[i] = Handle_SS_Auction_INVALID;
		GSS_AuctionPacketHandler[SS_Auction::PacketType::PKT_RES_AUCTION_LIST] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<SS_Auction::RES_AUCTION_LIST>(Handle_RES_AUCTION_LIST, session, buffer, offset);
			};
		GSS_AuctionPacketHandler[SS_Auction::PacketType::PKT_RES_RECEIPT_LIST] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<SS_Auction::RES_RECEIPT_LIST>(Handle_RES_RECEIPT_LIST, session, buffer, offset);
			};
		GSS_AuctionPacketHandler[SS_Auction::PacketType::PKT_RES_REGIST_ITEM] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<SS_Auction::RES_REGIST_ITEM>(Handle_RES_REGIST_ITEM, session, buffer, offset);
			};
		GSS_AuctionPacketHandler[SS_Auction::PacketType::PKT_RES_PURCHASE_ITEM] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<SS_Auction::RES_PURCHASE_ITEM>(Handle_RES_PURCHASE_ITEM, session, buffer, offset);
			};
		GSS_AuctionPacketHandler[SS_Auction::PacketType::PKT_RES_RECEIPT_ITEM] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return DispatchPacket<SS_Auction::RES_RECEIPT_ITEM>(Handle_RES_RECEIPT_ITEM, session, buffer, offset);
			};
	}

	static bool HandlePacket(SessionPtr& session, const PacketHeader& header, char* ptr, size_t size)
	{
		boost::asio::mutable_buffer buffer = boost::asio::buffer(ptr, size);
		int offset = 4;

		return GSS_AuctionPacketHandler[header.Code](session, buffer, offset);
	}

private:
	template<typename PacketType, typename ProcessFunc>
	static bool DispatchPacket(ProcessFunc func, SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
	{
		PacketType pkt;
		if (!PacketUtil::Parse(pkt, buffer, buffer.size(), offset))
		{
			#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
				UE_LOG(LogTemp, Warning, TEXT("Failed to Handle Packet"))
			#else
				spdlog::error("Failed to Handle Packet");
			#endif
			return false;
		}

		return func(session, pkt);
	}
};