#pragma once
#include "../Protocol/LobbyProtocol.pb.h"
#include "Packet.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "J1.h"
#endif

bool Handle_REQ_CHECK_TOKENVALID(SessionPtr& session, Game::REQ_CHECK_TOKENVALID& pkt);
bool Handle_REQ_CHARACTER_LIST(SessionPtr& session, Game::REQ_CHARACTER_LIST& pkt);
bool Handle_REQ_CHECK_NICKNAME(SessionPtr& session, Game::REQ_CHECK_NICKNAME& pkt);
bool Handle_REQ_CREATE_CHARACTER(SessionPtr& session, Game::REQ_CREATE_CHARACTER& pkt);
bool Handle_REQ_DELETE_CHARACTER(SessionPtr& session, Game::REQ_DELETE_CHARACTER& pkt);
bool Handle_REQ_ENTER_GAME(SessionPtr& session, Game::REQ_ENTER_GAME& pkt);
bool Handle_REQ_LEAVE_GAME(SessionPtr& session, Game::REQ_LEAVE_GAME& pkt);
