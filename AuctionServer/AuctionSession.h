#pragma once

#include "Session.h"

class AuctionSession : public Session
{
public:
	AuctionSession(asio::io_context& io_context);
	~AuctionSession();
	virtual SessionPtr GetSessionPtr() { return static_pointer_cast<Session>(shared_from_this()); }

public:
	virtual void AsyncWrite(const BufferPooledVector& data, size_t size) override;

	virtual void HandlePacket() override;

	void SendPacket(google::protobuf::Message& msg, const short packetCode);

};
