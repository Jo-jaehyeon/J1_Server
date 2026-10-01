#include "pch.h"
#include "GameSession.h"
#include "GameMember.h"
#include "Network/Handlers/GamePacketHandler.h"
#include "Utils/GameSessionManager.h"

GameSession::GameSession(asio::io_context& io_context)
	: Session(io_context)
{
}

GameSession::~GameSession()
{
}

bool GameSession::Close()
{
	if (!Session::Close())
		return false; // 이미 종료 처리된 세션이면 여기서 끝 — 중복 Unregister 방지

	GameSessionManager::Instance().Unregister(GetSessionId());
	spdlog::info("Session {} Disconnected", GetSessionId());

	return true;
}

void GameSession::AsyncWrite(const BufferPooledVector& data, size_t size)
{
	Session::AsyncWrite(data, size);
}

void GameSession::HandlePacket()
{
	SessionPtr session = this->GetSessionPtr();
	GamePacketHandler::HandlePacket(session, _header, reinterpret_cast<char*>(_recvBodyBuffer.data()), _recvBodyBuffer.size());
	_offset = 0;
}

void GameSession::SendPacket(google::protobuf::Message& msg, const short packetCode)
{
	auto bufSize = PacketUtil::RequiredSize(msg);
	BufferPooledVector vector_buffer(bufSize);
	PacketUtil::Serialize(asio::buffer(vector_buffer), packetCode, msg);
	Send(vector_buffer, bufSize);
}
