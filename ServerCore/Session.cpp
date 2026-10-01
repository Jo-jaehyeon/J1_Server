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
    auto data_ptr = std::make_shared<BufferPooledVector>(data);

    async_write(_socket,
        asio::buffer(data_ptr->data(), data_ptr->size()),
        bind_executor(_strand, [data_ptr, this, self = shared_from_this()](const boost::system::error_code& error, const size_t bytes_transferred)
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
        Close();
    }
}