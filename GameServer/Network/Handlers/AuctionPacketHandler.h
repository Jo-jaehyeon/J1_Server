#pragma once
#include "../Protocol/AuctionProtocol.pb.h"
#include "Packet.h"

bool Handle_REQ_OPENAUCTION(SessionPtr& session, Game::REQ_OPENAUCTION& pkt);
