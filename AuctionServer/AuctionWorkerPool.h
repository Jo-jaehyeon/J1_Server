#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include <boost/asio.hpp>

namespace active911 { class MySQLConnection; }

// ------------------------------------------------------------
//  Worker : io_context(작업 큐) 1개 + 스레드 1개 + DB 커넥션 1개
// ------------------------------------------------------------
class Worker
{
public:
	explicit Worker(std::string n)
		: name(std::move(n))
		, guard(boost::asio::make_work_guard(io)) // 큐가 비어도 run()이 끝나지 않게 붙잡아 둔다
	{
	}

	std::string name;
	boost::asio::io_context io;							// 이 io_context는 워커 스레드 1개만 run() 한다 (선언 순서: guard보다 앞)
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard;
	std::shared_ptr<active911::MySQLConnection> conn;   // 이 워커 전용. 다른 스레드는 만지지 않는다
	std::thread thread;
};

class AuctionWorkerPool
{
public:
	using Conn = active911::MySQLConnection;
	using Task = std::function<void(Conn&)>;

	static AuctionWorkerPool& Instance();

	AuctionWorkerPool(const AuctionWorkerPool&) = delete;
	AuctionWorkerPool& operator=(const AuctionWorkerPool&) = delete;

	// GConnectionPool에서 커넥션을 빌려 워커에 하나씩 고정
	void Start(size_t readWorkers, size_t writeWorkers);

	// 큐에 남은 작업을 모두 처리한 뒤 스레드를 종료, 커넥션을 반납.
	void Stop();

	void PostRead(Task task);                   // 읽기 워커에 라운드로빈
	void PostWrite(uint64 key, Task task);      // 쓰기 워커[key % 쓰기 워커 수]

private:
	AuctionWorkerPool();
	~AuctionWorkerPool();

	std::unique_ptr<Worker> Spawn(std::string name);
	void RunWorker(Worker& worker);
	static void Post(Worker& worker, Task task);

private:
	std::vector<std::unique_ptr<Worker>> _readers;
	std::vector<std::unique_ptr<Worker>> _writers;
	std::atomic<size_t>                  _readRoundRobin{ 0 };
	sql::Driver* _driver = nullptr;
};

