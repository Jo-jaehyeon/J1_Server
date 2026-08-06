#include "pch.h"
#include "ChatServer.h"
#include "ChatSession.h"
#include "Network/Handlers/ChatPacketHandler.h"

ChatServer::ChatServer(asio::io_context& io_context, int port)
	: _acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
	_io_context(io_context)
{

}

void ChatServer::StartAccept()
{
	ChatSession* session = new ChatSession(_io_context);
	ChatSessionPtr sessionPtr(session);

	//sessionPtr = sessionPtr의 이유
	//ChatSessionPtr& abc = SessionPtr은 주소값을 복사해오기 때문에 레퍼 카운팅이 증가하지 않음
	//람다에 &sessionPtr을 하게 될 경우 마찬가지로 레퍼카운팅이 증가하지 않아 OnAccept 중간에 크래시가 날 수 있음
	//의도적인 복사를 통해 넘겨주는것
	_acceptor.async_accept(sessionPtr->GetSocket(),
		boost::bind(
			&ChatServer::OnAccept, this,
			sessionPtr,
			boost::asio::placeholders::error
		));
}

void ChatServer::OnAccept(SessionPtr session, boost::system::error_code ec)
{
	if (!ec)
	{
		spdlog::info("Session Connected");
		session->Start();
	}
	StartAccept();
}

int main()
{
	ChatPacketHandler::Init();

	try
	{
		int port = 9002;
		boost::asio::io_context io_context;
		ChatServer s(io_context, port);
		s.StartAccept();
		spdlog::info("Server Start {}", port);
		io_context.run();
	}
	catch (std::exception& e)
	{
		spdlog::info("Exception {}", e.what());
	}

	return 0;
}