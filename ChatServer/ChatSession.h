#pragma once
#include "Session.h"

class ChatSession : public Session
{
public:
	ChatSession(asio::io_context& io_context);
	~ChatSession();
	virtual SessionPtr GetSessionPtr() { return static_pointer_cast<Session>(shared_from_this()); }

public:
	virtual void AsyncWrite(const BufferPooledVector& data, size_t size) override;

	virtual void HandlePacket() override;


private:
	std::string _name;
};
