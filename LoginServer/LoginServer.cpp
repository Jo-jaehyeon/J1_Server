#include "pch.h"
#include "LoginServer.h"
#include "LoginSession.h"
#include "Network/Handlers/LoginPacketHandler.h"
#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "DB/ConnectionFactory.h"

LoginServer::LoginServer(asio::io_context& io_context, int port)
	: _acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
	_io_context(io_context)
{
}

void LoginServer::StartAccept()
{
	LoginSession* session = new LoginSession(_io_context);
	LoginSessionPtr sessionPtr(session);

	//sessionPtr = sessionPtr의 이유
	//ChatSessionPtr& abc = SessionPtr은 주소값을 복사해오기 때문에 레퍼 카운팅이 증가하지 않음
	//람다에 &sessionPtr을 하게 될 경우 마찬가지로 레퍼카운팅이 증가하지 않아 OnAccept 중간에 크래시가 날 수 있음
	//의도적인 복사를 통해 넘겨주는것
	_acceptor.async_accept(sessionPtr->GetSocket(),
		boost::bind(
			&LoginServer::OnAccept, this,
			sessionPtr,
			boost::asio::placeholders::error
		));
}

void LoginServer::OnAccept(SessionPtr session, boost::system::error_code ec)
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
	LoginPacketHandler::Init();

	// DB ConnectionPool 생성
	std::shared_ptr<active911::MySQLConnectionFactory>connection_factory(new active911::MySQLConnectionFactory("localhost:3306", "root", "OmegaAlpha"));
	active911::ConnectionPool<active911::MySQLConnection>::Init(10, connection_factory);

	try
	{
		int port = 9000;
		boost::asio::io_context io_context;
		LoginServer s(io_context, port);
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

//try {
//	// MySQL 드라이버 인스턴스를 가져옵니다.
//	sql::Driver* driver = get_driver_instance();
//
//	// 데이터베이스에 연결합니다.
//	const std::string server = "tcp://127.0.0.1:3306";
//	const std::string name = "root";
//	const std::string password = "OmegaAlpha";
//	std::unique_ptr<sql::Connection> conn(driver->connect(server, name, password));
//
//	// 데이터베이스 작업을 수행합니다.
//	conn->setSchema("J1_DB");
//
//	// 쿼리 실행
//	std::string select_sql = "INSERT INTO Test(a, b) VALUES(9, 10)";
//	std::unique_ptr<sql::Statement> stmt(conn->createStatement());
//	int affetedRows = stmt->executeUpdate(select_sql);
//	std::cout << affetedRows << std::endl;
//	std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT * FROM Test"));
//
//	// 결과 처리
//	while (res && res->next()) {
//		std::cout << res->getString(1) << " " << res->getString(2) << std::endl;
//	}
//
//}
//catch (sql::SQLException& e) {
//	std::cerr << "SQLException: " << e.what() << std::endl;
//	std::cerr << "Error code: " << e.getErrorCode() << std::endl;
//	std::cerr << "SQL state: " << e.getSQLState() << std::endl;
//}
//catch (std::exception& e) {
//	std::cerr << "Exception: " << e.what() << std::endl;
//}
//catch (...) {
//	std::cerr << "Unknown exception occurred" << std::endl;
//}