#include "pch.h"
#include "Session.h"

#pragma message("Session.cpp is being compiled")

Session::Session(boost::asio::io_context& io_context)
	: _socket(io_context), _strand(boost::asio::make_strand(io_context))
{
	memset(_recvBuffer, 0, sizeof(_recvBuffer));
	memset(_sendBuffer, 0, sizeof(_sendBuffer));
    _offset = 0;
}

void Session::Start()
{
    AsyncRead();
}

void Session::AsyncRead()
{
    memset(_recvBuffer, 0, RecvBufferSize);
    asio::async_read(_socket,
        asio::buffer(_recvBuffer, RecvBufferSize),
        asio::bind_executor(_strand, [this, self = shared_from_this()](const boost::system::error_code& error, const size_t bytes_transferred)
            {
                OnRead(error, bytes_transferred);
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

void Session::OnRead(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (!err)
    {
        spdlog::trace("Received Packet bytes {}", bytes_transferred);

        // Header Packet Deserializa
        asio::mutable_buffer buffer = asio::buffer(_recvBuffer, RecvBufferSize);

        if (PacketUtil::ParseHeader(buffer, &_header, _offset))
        {
            spdlog::trace("Received Header info -> Code : {}, Length : {}", _header.Code, _header.Length);
            HandlePacket();
        }
        else
        {
            spdlog::error("Failed to parse Header..");
        }
    }
    else
    {
        spdlog::error("Packet Read Error : {}", err.message());
    }

    AsyncRead();
}

void Session::OnWrite(const boost::system::error_code& err, size_t bytes_transferred)
{

}