#pragma once
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>

#include "Packet.h"

using namespace boost;

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(boost::asio::io_context& io_context);
	~Session() {};

	boost::asio::ip::tcp::socket& GetSocket() { return _socket; }
	void Start();
	void Send(BufferPooledVector& buffer, size_t size);

protected:
	virtual void AsyncRead();
	virtual void AsyncHeaderRead();
	virtual void AsyncBodyRead();
	virtual void AsyncWrite(const BufferPooledVector& data, size_t size);

	void OnHeaderRead(const boost::system::error_code& err, size_t bytes_transferred);
	void OnBodyRead(const boost::system::error_code& err, size_t bytes_transferred);
	void OnWrite(const boost::system::error_code& err, size_t bytes_transferred);

	virtual void HandlePacket() = 0;

protected:
	boost::asio::ip::tcp::socket _socket;
	boost::asio::strand<boost::asio::io_context::executor_type> _strand;

	PacketHeader _header;
	int _offset;
	BufferPooledVector _recvBodyBuffer;

private:
	const static size_t RecvBufferSize = 1024;
	const static size_t SendBufferSize = 1024;
	const static size_t HeaderBufferSize = 4;
	char _recvBuffer[RecvBufferSize];
	char _sendBuffer[SendBufferSize];
};