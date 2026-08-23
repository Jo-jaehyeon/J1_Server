#include "pch.h"
#include "AuctionSession.h"
#include "Network/Handlers/AuctionPacketHandler.h"

AuctionSession::AuctionSession(asio::io_context& io_context)
	: Session(io_context)
{
}

AuctionSession::~AuctionSession()
{
}

void AuctionSession::AsyncWrite(const BufferPooledVector& data, size_t size)
{
	Session::AsyncWrite(data, size);
}

void AuctionSession::HandlePacket()
{
	SessionPtr session = this->GetSessionPtr();
	AuctionPacketHandler::HandlePacket(session, _header, reinterpret_cast<char*>(_recvBodyBuffer.data()), _recvBodyBuffer.size());
	_offset = 0;
}

void AuctionSession::SendPacket(google::protobuf::Message& msg, const short packetCode)
{
	auto bufSize = PacketUtil::RequiredSize(msg);
	BufferPooledVector vector_buffer(bufSize);
	PacketUtil::Serialize(asio::buffer(vector_buffer), packetCode, msg);
	Send(vector_buffer, bufSize);
}
