#pragma once
#include "../Protocol/AuctionProtocol.pb.h"
#include "Packet.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "J1.h"
#endif

bool Handle_REQ_AUCTION_LIST(SessionPtr& session, Game::REQ_AUCTION_LIST& pkt);
bool Handle_REQ_RECEIPT_LIST(SessionPtr& session, Game::REQ_RECEIPT_LIST& pkt);
bool Handle_REQ_REGIST_ITEM(SessionPtr& session, Game::REQ_REGIST_ITEM& pkt);
bool Handle_REQ_PURCHASE_ITEM(SessionPtr& session, Game::REQ_PURCHASE_ITEM& pkt);
bool Handle_REQ_RECEIPT_ITEM(SessionPtr& session, Game::REQ_RECEIPT_ITEM& pkt);
