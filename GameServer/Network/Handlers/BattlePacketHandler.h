#pragma once
#include "../Protocol/BattleProtocol.pb.h"
#include "Packet.h"

bool Handle_REQ_ATTACK(SessionPtr& session, Game::REQ_ATTACK& pkt);
