#include "pch.h"
#include "Session.h"

#pragma message("Session.cpp is being compiled")

Session::Session(boost::asio::io_context& io_context)
	: _socket(io_context), _strand(boost::asio::make_strand(io_context)), _offset(0)
{
	memset(_recvBuffer, 0, sizeof(_recvBuffer));
	memset(_sendBuffer, 0, sizeof(_sendBuffer));
}

void Session::Start()
{
    AsyncRead();
}

bool Session::Close()
{
    bool expected = false;
    if (!_closed.compare_exchange_strong(expected, true))
        return false; // 이미 처리됐으면 재실행 안 함

    boost::system::error_code ec;
    _socket.close(ec);

    return true;
}

void Session::Send(BufferPooledVector& buffer, size_t size)
{
    AsyncWrite(buffer, size);
}

void Session::AsyncRead()
{
    AsyncHeaderRead();
}

void Session::AsyncHeaderRead()
{
    _offset = 0;
    memset(_recvBuffer, 0, HeaderBufferSize);
    asio::async_read(_socket,
        asio::buffer(_recvBuffer, HeaderBufferSize),
        asio::bind_executor(_strand, [this, self = shared_from_this()](const boost::system::error_code& error, const size_t bytes_transferred)
            {
                OnHeaderRead(error, bytes_transferred);
            }
        ));
}

void Session::AsyncBodyRead()
{
    _recvBodyBuffer.clear();
    _recvBodyBuffer.resize(_header.Length);
    
    asio::async_read(_socket,
        asio::buffer(_recvBodyBuffer.data(), _recvBodyBuffer.size()),
        asio::bind_executor(_strand, [this, self = shared_from_this()](const boost::system::error_code& error, const size_t bytes_transferred)
            {
                OnBodyRead(error, bytes_transferred);
            }
        ));
}

void Session::AsyncWrite(const BufferPooledVector& data, size_t size)
{
    // 호출 스레드에서 복사해 두고, 쓰기는 strand에 맡긴다
    EnqueueSend(std::make_shared<const BufferPooledVector>(data));
}

void Session::SendShared(std::shared_ptr<const BufferPooledVector> buffer)
{
    if (!buffer || buffer->empty())
        return;

    EnqueueSend(std::move(buffer));
}

void Session::EnqueueSend(std::shared_ptr<const BufferPooledVector> data)
{
    // strand 안에서 호출되면 바로 실행되고, 다른 스레드에서 호출되면 strand에 예약된다.
    // -> 같은 소켓에 async_write가 동시에 걸리지 않는다.
    asio::dispatch(_strand, [this, self = shared_from_this(), data = std::move(data)]() mutable
        {
            if (_sendQueue.size() >= MaxSendQueue)
            {
                spdlog::warn("{} session send queue overflow -> close", GetSessionId());
                _sendQueue.clear();
                _writing = false;
                Close();
                return;
            }

            _sendQueue.push_back(std::move(data));
            if (!_writing)
                DoWrite();
        });
}

void Session::DoWrite()
{
    _writing = true;
    auto data = _sendQueue.front();

    asio::async_write(_socket,
        asio::buffer(data->data(), data->size()),
        asio::bind_executor(_strand, [this, self = shared_from_this(), data](const boost::system::error_code& error, const size_t bytes_transferred)
            {
                OnWrite(error, bytes_transferred);
            }));
}

void Session::OnHeaderRead(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (!err)
    {
        spdlog::trace("Received Header bytes {}", bytes_transferred);

        // Header Packet Deserialize
        asio::mutable_buffer buffer = asio::buffer(_recvBuffer, HeaderBufferSize);

        if (PacketUtil::ParseHeader(buffer, &_header, _offset))
        {
            spdlog::trace("Received Header info -> Code : {}, Length : {}", _header.Code, _header.Length);
            AsyncBodyRead();
        }
        else
        {
            // 버그가 발생한 클라 or 악의적인 접근
            spdlog::error("Failed to parse Header..");
            Close();
        }
    }
    else
    {
        spdlog::error("{} session Header Read Error : {}", session_id, err.message());
        Close();
    }
}

void Session::OnBodyRead(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (!err)
    {
        spdlog::trace("Received Body bytes {}", bytes_transferred);
        HandlePacket();
        AsyncHeaderRead();
    }
    else
    {
        spdlog::error("Packet Read Error : {}", err.message());
        Close();
    }
}

void Session::OnWrite(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (err)
    {
        _sendQueue.clear();
        _writing = false;
        Close();
        return;
    }

    _sendQueue.pop_front();
    if (_sendQueue.empty())
        _writing = false;
    else
        DoWrite();
}