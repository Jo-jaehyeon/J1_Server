#include "pch.h"
#include "AuctionServer.h"
#include "AuctionSession.h"
#include "Network/Handlers/AuctionPacketHandler.h"
#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "DB/ConnectionFactory.h"
#include "AuctionWorkerPool.h"

namespace
{
	// 워커 구성: 커넥션 풀 크기 = 읽기 + 쓰기 + 여유분
	// 쓰기 워커 수는 부하 테스트로 정한다. (4 -> 8 -> 12 -> 16 으로 바꿔 가며 TPS가 꺾이는 지점 확인)
	constexpr size_t kReadWorkers = 1;
	constexpr size_t kWriteWorkers = 20;
	constexpr size_t kSpareConnections = 2;     // 만료 배치 등 나중에 생길 백그라운드 작업용
}

AuctionServer::AuctionServer(asio::io_context& io_context, int port)
	: _acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
	_io_context(io_context)
{

}

void AuctionServer::StartAccept()
{
	AuctionSession* session = new AuctionSession(_io_context);
	AuctionSessionPtr sessionPtr(session);

	//sessionPtr = sessionPtr의 이유
	//AuctionSessionPtr& abc = SessionPtr은 주소값을 복사해오기 때문에 레퍼 카운팅이 증가하지 않음
	//람다에 &sessionPtr을 하게 될 경우 마찬가지로 레퍼카운팅이 증가하지 않아 OnAccept 중간에 크래시가 날 수 있음
	//의도적인 복사를 통해 넘겨주는것
	_acceptor.async_accept(sessionPtr->GetSocket(),
		boost::bind(
			&AuctionServer::OnAccept, this,
			sessionPtr,
			boost::asio::placeholders::error
		));
}

void AuctionServer::OnAccept(SessionPtr session, boost::system::error_code ec)
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
	AuctionPacketHandler::Init();

	// DB ConnectionPool 생성
	std::shared_ptr<active911::MySQLConnectionFactory>connection_factory(new active911::MySQLConnectionFactory("localhost:3306", "root", "OmegaAlpha"));
	active911::ConnectionPool<active911::MySQLConnection>::Init(kReadWorkers + kWriteWorkers + kSpareConnections, connection_factory);

	try
	{
		// DB 워커 시작 (커넥션을 워커마다 1개씩 고정). 네트워크 io 스레드는 아래 run() 하나뿐이다.
		AuctionWorkerPool::Instance().Start(kReadWorkers, kWriteWorkers);

		int port = 9003;
		boost::asio::io_context io_context;
		AuctionServer s(io_context, port);
		s.StartAccept();
		spdlog::info("Server Start {}", port);
		io_context.run();
	}
	catch (std::exception& e)
	{
		spdlog::info("Server Exception {}", e.what());
	}

	// 정상 종료든 예외든 워커는 여기서 정리한다 (큐에 남은 작업을 끝낸 뒤 join)
	AuctionWorkerPool::Instance().Stop();

	return 0;
}