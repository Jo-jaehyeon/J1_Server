#include "pch.h"
#include "LoginPacketHandler.h"
#include "LoginSession.h"
#include "SignManager.h"

LoginHandlerFunc GLoginPacketHandler[UINT16_MAX];

bool Handle_Login_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& len)
{
    return false;
}

bool Handle_REQ_LOGIN(SessionPtr& session, Login::REQ_LOGIN& pkt)
{
    const std::string& id = pkt.id();
    const std::string& pw = pkt.pw();
    int account_id = GSignManager->TrySignIn(id, pw);

    Login::RES_LOGIN loginPkt;
    loginPkt.set_player_id(account_id);

    if (LoginSessionPtr _Session = static_pointer_cast<LoginSession>(session))
        _Session->SendPacket(loginPkt, Login::PacketType::PKT_RES_LOGIN);

    return true;
}

bool Handle_REQ_CHECK_ID(SessionPtr& session, Login::REQ_CHECK_ID& pkt)
{
    const std::string& id = pkt.id();
    bool success = GSignManager->TryCheckID(id);

    Login::RES_CHECK_ID checkPkt;
    checkPkt.set_result(success);
    
    if(LoginSessionPtr _Session = static_pointer_cast<LoginSession>(session))
        _Session->SendPacket(checkPkt, Login::PacketType::PKT_RES_CHECK_ID);

    return true;
}

bool Handle_REQ_JOIN(SessionPtr& session, Login::REQ_JOIN& pkt)
{
    const std::string& id = pkt.id();
    const std::string& pw = pkt.pw();
    bool success = GSignManager->TrySignUp(id, pw);

    Login::RES_JOIN joinPkt;
    joinPkt.set_result(success);

    if (LoginSessionPtr _Session = static_pointer_cast<LoginSession>(session))
        _Session->SendPacket(joinPkt, Login::PacketType::PKT_RES_JOIN);

    return success;
}