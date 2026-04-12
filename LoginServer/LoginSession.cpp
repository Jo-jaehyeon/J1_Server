#include "pch.h"
#include "LoginSession.h"


LoginSession::LoginSession(asio::io_context& io_context)
	: Session(io_context)
{
}

LoginSession::~LoginSession()
{
}

void LoginSession::AsyncWrite(const BufferPooledVector& data, size_t size)
{
	Session::AsyncWrite(data, size);
}

void LoginSession::HandlePacket()
{
	SessionPtr session = this->GetSessionPtr();
	//LoginPacketHandler::HandlePacket(session, _header, reinterpret_cast<char*>(_recvBodyBuffer.data()), _recvBodyBuffer.size());
	_offset = 0;
}

void LoginSession::SendPacket(google::protobuf::Message& msg, const short packetCode)
{
	// TODO : Session으로 옮기기
	auto bufSize = PacketUtil::RequiredSize(msg);
	BufferPooledVector vector_buffer(bufSize);
	PacketUtil::Serialize(asio::buffer(vector_buffer), packetCode, msg);
	Send(vector_buffer, bufSize);
}
