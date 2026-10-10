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
	virtual bool Close();
	void Send(BufferPooledVector& buffer, size_t size);

	// 이미 만들어진(공유) 패킷 버퍼를 복사 없이 전송 큐에 넣는다. 어느 스레드에서 호출해도 안전하다.
	void SendShared(std::shared_ptr<const BufferPooledVector> buffer);

	void SetSessionId(uint64 id) { session_id = id; }
	uint64 GetSessionId() const { return session_id; }

protected:
	virtual void AsyncRead();
	virtual void AsyncHeaderRead();
	virtual void AsyncBodyRead();
	virtual void AsyncWrite(const BufferPooledVector& data, size_t size);

	// 전송 큐 진입점. 호출 스레드와 무관하게 strand 위에서 큐에 쌓고 순서대로 쓴다.
	void EnqueueSend(std::shared_ptr<const BufferPooledVector> data);
	void DoWrite();                       // strand 안에서만 호출

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

	std::atomic<bool> _closed{ false }; // Close() 중복 실행 방지용

	// 전송 큐: strand 안에서만 접근한다 (락 없음)
	static constexpr size_t MaxSendQueue = 4096;   // 이 이상 쌓이면 느린 클라이언트로 보고 끊는다
	std::deque<std::shared_ptr<const BufferPooledVector>> _sendQueue;
	bool _writing = false;

	uint64 session_id = 0;
};