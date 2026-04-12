#pragma once
#include "Session.h"

class LoginSession : public Session
{
public:
	LoginSession(asio::io_context& io_context);
	~LoginSession();
	virtual SessionPtr GetSessionPtr() { return static_pointer_cast<Session>(shared_from_this()); }

public:
	virtual void AsyncWrite(const BufferPooledVector& data, size_t size) override;

	virtual void HandlePacket() override;

	void SendPacket(google::protobuf::Message& msg, const short packetCode);

};

