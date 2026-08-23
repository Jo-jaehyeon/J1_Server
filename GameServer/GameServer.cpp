#include "pch.h"
#include "GameServer.h"
#include "GameSession.h"
#include "AuctionSession.h"
#include "Network/Handlers/GamePacketHandler.h"
#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "DB/ConnectionFactory.h"

GameServer::GameServer(asio::io_context& io_context, int port)
	: _acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
	_io_context(io_context)
{
}

void GameServer::StartAccept()
{
	GameSession* session = new GameSession(_io_context);
	GameSessionPtr sessionPtr(session);

	//sessionPtr = sessionPtr의 이유
	//ChatSessionPtr& abc = SessionPtr은 주소값을 복사해오기 때문에 레퍼 카운팅이 증가하지 않음
	//람다에 &sessionPtr을 하게 될 경우 마찬가지로 레퍼카운팅이 증가하지 않아 OnAccept 중간에 크래시가 날 수 있음
	//의도적인 복사를 통해 넘겨주는것
	_acceptor.async_accept(sessionPtr->GetSocket(),
		boost::bind(
			&GameServer::OnAccept, this,
			sessionPtr,
			boost::asio::placeholders::error
		));
}

void GameServer::OnAccept(SessionPtr session, boost::system::error_code ec)
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
	GamePacketHandler::Init();

	// DB ConnectionPool 생성
	std::shared_ptr<active911::MySQLConnectionFactory>connection_factory(new active911::MySQLConnectionFactory("localhost:3306", "root", "OmegaAlpha"));
	active911::ConnectionPool<active911::MySQLConnection>::Init(10, connection_factory);

	try
	{
		int port = 9001;
		boost::asio::io_context io_context;
		GameServer s(io_context, port);
		s.StartAccept();
		spdlog::info("Server Start {}", port);
		io_context.run();


		// 경매장 서버와 연결
		boost::asio::io_context auction_context;
		AuctionSession* session = new AuctionSession(auction_context);
		AuctionSessionPtr sessionPtr(session);

		sessionPtr->Connect("127.0.0.1", 9002);
	}
	catch (std::exception& e)
	{
		spdlog::info("Server Exception {}", e.what());
	}

	return 0;
}