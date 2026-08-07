#include "pch.h"
#include "GamePacketHandler.h"
#include "GameSession.h"
#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "SqlUtils.h"

GameHandlerFunc GGamePacketHandler[UINT16_MAX];

bool Handle_Game_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset)
{
	return false;
}

bool Handle_REQ_CHARACTER_LIST(SessionPtr& session, Game::REQ_CHARACTER_LIST& pkt)
{
    const uint64 id = pkt.id();
    const std::string& token = pkt.token();
    
    // token 체크
    auto conn = GConnectionPool->borrow();
    while (conn == nullptr)
    {
        conn = GConnectionPool->borrow();
    }
    std::string Query_CheckID = "SELECT 1 "
        "FROM sessions "
        "WHERE account_id = ? AND token = ? AND expires_at > NOW() "
        "LIMIT 1";
    auto result_set = SqlUtils::executeQuery(conn->sql_connection, "J1_DB", Query_CheckID, id, token);
    bool result = result_set->next();

    int successDelete = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", "DELETE FROM sessions WHERE account_id = ? AND token = ?", id, token);
    if(successDelete < 0)   spdlog::info("Failed to update id info");

    Game::RES_CHARACTER_LIST charList_Pkt;
    charList_Pkt.set_result(result);         
    
    // TODO 
    //charList_Pkt.add_characters();

    if (GameSessionPtr _Session = static_pointer_cast<GameSession>(session))
        _Session->SendPacket(charList_Pkt, Game::PacketType::PKT_RES_CHARACTER_LIST);

    GConnectionPool->unborrow(conn);

    return true;
}

bool Handle_REQ_ENTER_GAME(SessionPtr& session, Game::REQ_ENTER_GAME& pkt)
{
	return false;
}

bool Handle_REQ_LEAVE_GAME(SessionPtr& session, Game::REQ_LEAVE_GAME& pkt)
{
	return false;
}
