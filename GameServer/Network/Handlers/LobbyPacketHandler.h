#pragma once
#include "../Protocol/LobbyProtocol.pb.h"
#include "Packet.h"

bool Handle_REQ_CHARACTER_LIST(SessionPtr& session, Game::REQ_CHARACTER_LIST& pkt);
bool Handle_REQ_ENTER_GAME(SessionPtr& session, Game::REQ_ENTER_GAME& pkt);
bool Handle_REQ_LEAVE_GAME(SessionPtr& session, Game::REQ_LEAVE_GAME& pkt);
