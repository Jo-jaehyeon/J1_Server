#include "pch.h"
#include "LobbyPacketHandler.h"
#include "GameSession.h"
#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "SqlUtils.h"
#include "Utils/ObjectUtils.h"
#include "GameSession.h"
#include "GameMember.h"

bool Handle_REQ_CHECK_TOKENVALID(SessionPtr& session, Game::REQ_CHECK_TOKENVALID& pkt)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	// 로그인 유효성 검증
	string checkToken = "SELECT 1 "
		"FROM sessions "
		"WHERE account_id = ? AND token = ? AND expires_at > NOW()"
		"LIMIT 1";
	int id = pkt.id();
	auto check_result = SqlUtils::executeQuery(conn->sql_connection, "J1_DB", checkToken, id, pkt.token());

	bool result = check_result->next();

	Game::RES_CHECK_TOKENVALID tokenPkt;
	tokenPkt.set_result(result);

	if (GameSessionPtr _Session = static_pointer_cast<GameSession>(session))
		_Session->SendPacket(tokenPkt, Game::PacketType::PKT_RES_CHECK_TOKENVALID);

	GConnectionPool->unborrow(conn);

	return true;
}

bool Handle_REQ_CHARACTER_LIST(SessionPtr& session, Game::REQ_CHARACTER_LIST& pkt)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	Game::RES_CHARACTER_LIST ListPkt;

	// 캐릭터 리스트 찾기
	if (pkt.id() > 0)
	{
		string FindList = 
			"SELECT cb.character_id, cb.slot_index, cb.nickname, cb.class_type, cb.level, cs.upper_skin, cs.lower_skin, cs.weapon_skin "
			"FROM character_base cb "
			"JOIN character_combat cc ON cb.character_id = cc.character_id "
			"JOIN character_skin cs   ON cb.character_id = cs.character_id "
			"WHERE cb.account_id = ? "
			"ORDER BY cb.slot_index";
		auto list_result = SqlUtils::executeQuery(conn->sql_connection, "J1_DB", FindList, pkt.id());
		ListPkt.set_result(list_result > 0);

		if (list_result)
			while(list_result->next())
			{
				Game::LobbyCharacterInfo* temp = ListPkt.add_characters();
				temp->set_character_id(list_result->getInt("character_id"));
				temp->set_slot_id(list_result->getInt("slot_index"));
				temp->set_name(list_result->getString("nickname"));
				temp->set_classtype(list_result->getInt("class_type"));
				temp->set_level(list_result->getInt("level"));
				temp->set_upperskinid(list_result->getInt("upper_skin"));
				temp->set_lowerskinid(list_result->getInt("lower_skin"));
				temp->set_weaponskinid(list_result->getInt("weapon_skin"));
			}
	}

	if (GameSessionPtr _Session = static_pointer_cast<GameSession>(session))
		_Session->SendPacket(ListPkt, Game::PacketType::PKT_RES_CHARACTER_LIST);

	GConnectionPool->unborrow(conn);

	return true;
}

bool Handle_REQ_CHECK_NICKNAME(SessionPtr& session, Game::REQ_CHECK_NICKNAME& pkt)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	string query = "SELECT 1 "
		"FROM character_base "
		"WHERE nickname = ? "
		"LIMIT 1";
	
	auto result_set = SqlUtils::executeQuery(conn->sql_connection, "J1_DB", query, pkt.name());

	bool result = result_set->next();

	Game::RES_CHECK_NICKNAME checkNamePkt;
	checkNamePkt.set_result(result);

	if (GameSessionPtr _Session = static_pointer_cast<GameSession>(session))
		_Session->SendPacket(checkNamePkt, Game::PacketType::PKT_RES_CHECK_NICKNAME);

	if (result)	spdlog::info("can't make this id");
	else        spdlog::info("can make this id");

	GConnectionPool->unborrow(conn);

	return true;
}

bool Handle_REQ_CREATE_CHARACTER(SessionPtr& session, Game::REQ_CREATE_CHARACTER& pkt)
{
	// 캐릭 생성 요청이 한번에 한개가 넘어오지 않았다면 실패
	if (pkt.characters_size() != 1) return false;		

	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	bool result = false;
	try {
		// 오토커밋 끄기 -> 트랜잭션 시작
		conn->SetAutoCommit(false);

		string cbquery = "INSERT INTO character_base (account_id, slot_index, nickname, class_type) "
			"VALUES (?, ?, ?, ?)";
		string ccquery = "INSERT INTO character_combat (character_id) "
			"VALUES (?)";
		string csquery = "INSERT INTO character_skin (character_id, upper_skin, lower_skin, weapon_skin) "
			"VALUES (?, ?, ?, ?)";

		Game::LobbyCharacterInfo temp = pkt.characters(0);
		int character_id = SqlUtils::executeUpdate_GenKeys(conn->sql_connection, "J1_DB", cbquery, pkt.account_id(), temp.slot_id(), temp.name(), temp.classtype());
		int ccUpdate = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", ccquery, character_id);
		int csUpdate = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", csquery, character_id, temp.upperskinid(), temp.lowerskinid(), temp.weaponskinid());
		
		if (character_id > 0 && ccUpdate > 0 && csUpdate > 0)
		{
			conn->Commit();
			result = true;
		}
		else 
		{
			conn->Rollback();
		}
	}
	catch (const sql::SQLException& e) {
		// 실패 시 롤백
		conn->Rollback();
		spdlog::info("캐릭터 생성 실패, 롤백: {}", e.what());
	}

	// 오토커밋 복구
	conn->SetAutoCommit(true);
	GConnectionPool->unborrow(conn);

	// 결과 패킷 발송
	Game::RES_CREATE_CHARACTER createPkt;
	createPkt.set_result(result);

	if (GameSessionPtr _Session = static_pointer_cast<GameSession>(session))
		_Session->SendPacket(createPkt, Game::PacketType::PKT_RES_CREATE_CHARACTER);

	return true;
}

bool Handle_REQ_DELETE_CHARACTER(SessionPtr& session, Game::REQ_DELETE_CHARACTER& pkt)
{
	// 패킷의 내용이 하나라도 비었으면 실패
	if ((pkt.account_id() && pkt.character_id()) != true)			return false;

	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	bool result = false;
	try {
		// 오토커밋 끄기 -> 트랜잭션 시작
		conn->SetAutoCommit(false);

		string ccquery = "DELETE FROM character_combat "
			"WHERE character_id = ?";
		string csquery = "DELETE FROM character_skin "
			"WHERE character_id = ?";
		string ciquery = "DELETE FROM character_inventory "
			"WHERE character_id = ?";
		string cbquery = "DELETE FROM character_base "
			"WHERE character_id = ? AND account_id = ?;";

		int ccUpdate = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", ccquery, pkt.character_id());
		int csUpdate = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", csquery, pkt.character_id());
		int ciUpdate = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", ciquery, pkt.character_id());
		int cbUpdate = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", cbquery, pkt.character_id(), pkt.account_id());

		if (cbUpdate == 0)		
			conn->Rollback();
		else
		{
			conn->Commit();
			result = true;
		}
		
	}
	catch (const sql::SQLException& e) {
		// 실패 시 롤백
		conn->Rollback();
		spdlog::info("캐릭터 삭제 실패, 롤백: {}", e.what());
	}

	// 오토커밋 복구
	conn->SetAutoCommit(true);
	GConnectionPool->unborrow(conn);

	// 결과 패킷 발송
	Game::RES_DELETE_CHARACTER deletePkt;
	deletePkt.set_result(result);

	if (GameSessionPtr _Session = static_pointer_cast<GameSession>(session))
		_Session->SendPacket(deletePkt, Game::PacketType::PKT_RES_DELETE_CHARACTER);

	return true;
}

bool Handle_REQ_ENTER_GAME(SessionPtr& session, Game::REQ_ENTER_GAME& pkt)
{
	GameMemberPtr player = ObjectUtils::CreatePlayer(pkt.player_id(), pkt.name());

	GameSessionPtr cs = static_pointer_cast<GameSession>(session);
	player->session = cs;
	cs->player.store(player);

	GRoom->HandleEnterPlayerLocked(player);

	return true;
}

bool Handle_REQ_LEAVE_GAME(SessionPtr& session, Game::REQ_LEAVE_GAME& pkt)
{
	GameSessionPtr cs = static_pointer_cast<GameSession>(session);
	cs->player.store({});

	GRoom->HandleLeavePlayerLocked(pkt.player_id());

	return true;
}

