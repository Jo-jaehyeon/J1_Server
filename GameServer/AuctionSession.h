#pragma once

#include "Session.h"
#include "pch.h"

class AuctionSession : public Session
{
public:
	AuctionSession(asio::io_context& io_context);
	~AuctionSession();
	virtual SessionPtr GetSessionPtr() { return static_pointer_cast<Session>(shared_from_this()); }

public:
	virtual void AsyncWrite(const BufferPooledVector& data, size_t size) override;

	virtual void HandlePacket() override;

	void Connect(std::string host, int port);
	void Disconnect();
	void OnConnect(const boost::system::error_code& err);

	void SendPacket(google::protobuf::Message& msg, const short packetCode);

protected:
	asio::io_context* _io_context;
	boost::asio::ip::tcp::socket _socket;

	std::atomic<bool> _connected = false;
};

