#include "pch.h"
#include "ChatSession.h"
#include "Network/Handlers/ChatPacketHandler.h"

ChatSession::ChatSession(asio::io_context& io_context)
	: Session(io_context)
{
}

ChatSession::~ChatSession()
{
}

void ChatSession::AsyncWrite(const BufferPooledVector& data, size_t size)
{
	Session::AsyncWrite(data, size);
}

void ChatSession::HandlePacket()
{
	SessionPtr session = this->GetSessionPtr();
	ChatPacketHandler::HandlePacket(session, _header, reinterpret_cast<char*>(_recvBodyBuffer.data()), _recvBodyBuffer.size());
	_offset = 0;
}