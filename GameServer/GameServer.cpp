#include "pch.h"
#include "GameServer.h"
#include "GameSession.h"
#include "AuctionSession.h"
#include "Network/Handlers/GamePacketHandler.h"
#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "DB/ConnectionFactory.h"
#include "Utils/GameSessionManager.h"
#include "Utils/AuctionCache.h"

AuctionSessionPtr GAuctionSession;

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

void GameServer::OnAccept(GameSessionPtr session, boost::system::error_code ec)
{
	if (!ec)
	{
		spdlog::info("Session Connected");
		GameSessionManager::Instance().Register(session);
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


	boost::asio::io_context io_context;
	boost::asio::io_context auction_io;                            // 경매장 전용 스레드 (추가)
	auto auctionWork = boost::asio::make_work_guard(auction_io);   // 일이 없어도 run()이 끝나지 않게
	std::thread auctionThread;

	try
	{
		int port = 9001;
		GameServer s(io_context, port);
		s.StartAccept();
		spdlog::info("Server Start {}", port);

		// 경매장 서버와 연결
		GAuctionSession = std::make_shared<AuctionSession>(auction_io);
		AuctionCache::Instance().Init(auction_io);
		GAuctionSession->Connect("127.0.0.1", 9003);

		auctionThread = std::thread([&auction_io]()                      // 추가
			{
				try { auction_io.run(); }
				catch (std::exception& e) { spdlog::error("Auction thread exception {}", e.what()); }
			});

		io_context.run();
	}
	catch (std::exception& e)
	{
		spdlog::info("Server Exception {}", e.what());
	}

	// 종료 처리
	auctionWork.reset();   
	io_context.stop();
	auction_io.stop();
	if (auctionThread.joinable())
		auctionThread.join();

	return 0;
}