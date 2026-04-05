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

void ChatSession::SendPacket(google::protobuf::Message& msg, const short packetCode)
{
	auto bufSize = PacketUtil::RequiredSize(msg);
	BufferPooledVector vector_buffer(bufSize);
	PacketUtil::Serialize(asio::buffer(vector_buffer), packetCode, msg);
	Send(vector_buffer, bufSize);
}
