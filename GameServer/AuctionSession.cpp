#include "pch.h"
#include "AuctionSession.h"

#include "Network/Handlers/AuctionServerPacketHandler.h"

AuctionSession::AuctionSession(asio::io_context& io_context)
	: Session(io_context)
	, _io_context(&io_context)
	, _socket(io_context)
{
	AuctionServerPacketHandler::Init();
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
	AuctionServerPacketHandler::HandlePacket(session, _header, reinterpret_cast<char*>(_recvBodyBuffer.data()), _recvBodyBuffer.size());
	_offset = 0;
}

void AuctionSession::Connect(std::string host, int port)
{
	const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(host), port);
	_socket.async_connect(endpoint,
		boost::bind(
			&AuctionSession::OnConnect,
			this,
			boost::asio::placeholders::error
		)
	);
}

void AuctionSession::Disconnect()
{
	if (_connected.exchange(false) == false)	return;		// 이미 disconnect 처리됨

	boost::system::error_code ec;

	// 1. Graceful shutdown (양방향 송수신 중단을 상대방에게 알림)
	_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

	// 2. 소켓 자체를 닫음 (fd 반환)
	_socket.close(ec);
}

void AuctionSession::OnConnect(const boost::system::error_code& err)
{
	if (!err)
	{
		AsyncRead();

	}
	else
	{
	}
}

void AuctionSession::SendPacket(google::protobuf::Message& msg, const short packetCode)
{
	auto bufSize = PacketUtil::RequiredSize(msg);
	BufferPooledVector vector_buffer(bufSize);
	PacketUtil::Serialize(asio::buffer(vector_buffer), packetCode, msg);
	Send(vector_buffer, bufSize);
}
