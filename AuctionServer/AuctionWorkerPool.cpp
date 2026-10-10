#include "pch.h"
#include "AuctionWorkerPool.h"

#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"

AuctionWorkerPool::AuctionWorkerPool() = default;

AuctionWorkerPool::~AuctionWorkerPool()
{
	Stop();                                       // 스레드가 join 되지 않은 채 소멸하면 terminate 된다
}

AuctionWorkerPool& AuctionWorkerPool::Instance()
{
	static AuctionWorkerPool instance;
	return instance;
}

// ---------------------------------
//    시작 / 종료 (메인 스레드)
// ---------------------------------
void AuctionWorkerPool::Start(size_t readWorkers, size_t writeWorkers)
{
	if (!_readers.empty() || !_writers.empty())
		return;                                   // 이미 시작됨

	if (readWorkers == 0)  readWorkers = 1;
	if (writeWorkers == 0) writeWorkers = 1;

	// 드라이버는 여기(메인 스레드)에서 한 번만 얻는다. 워커 스레드에서 get_driver_instance()를 부르지 않는다.
	_driver = get_driver_instance();

	for (size_t i = 0; i < readWorkers; ++i)
		_readers.push_back(Spawn("auction-read-" + std::to_string(i)));

	for (size_t i = 0; i < writeWorkers; ++i)
		_writers.push_back(Spawn("auction-write-" + std::to_string(i)));

	spdlog::info("AuctionWorkerPool started : read {}, write {}", readWorkers, writeWorkers);
}

void AuctionWorkerPool::Stop()
{
	// io.stop()을 부르지 않는다. work guard만 풀어서, 큐에 남은 작업을 끝까지 처리하고 run()이 스스로 끝나게 한다.
	for (auto& w : _readers) w->guard.reset();
	for (auto& w : _writers) w->guard.reset();

	for (auto& w : _readers) if (w->thread.joinable()) w->thread.join();
	for (auto& w : _writers) if (w->thread.joinable()) w->thread.join();

	for (auto& w : _readers)
	{
		if (w->conn && GConnectionPool) GConnectionPool->unborrow(w->conn);
		w->conn.reset();
	}
	for (auto& w : _writers)
	{
		if (w->conn && GConnectionPool) GConnectionPool->unborrow(w->conn);
		w->conn.reset();
	}

	_readers.clear();
	_writers.clear();
}

std::unique_ptr<Worker> AuctionWorkerPool::Spawn(std::string name)
{
	auto worker = std::make_unique<Worker>(std::move(name));

	// 커넥션을 빌려서 워커가 끝날 때까지 쥐고 있는다. (매 요청마다 borrow/unborrow 하지 않는다)
	// 풀에 남은 커넥션이 없으면 ConnectionUnavailable 예외가 나간다.
	worker->conn = GConnectionPool->borrow();

	Worker* raw = worker.get();
	worker->thread = std::thread([this, raw]() { RunWorker(*raw); });
	return worker;
}

void AuctionWorkerPool::RunWorker(Worker& worker)
{
	// MySQL Connector/C++ : 커넥션을 쓰는 스레드는 시작/종료 때 호출해 주는 것이 권장된다.
	try { if (_driver) _driver->threadInit(); }
	catch (...) {}

	spdlog::info("{} started", worker.name);
	worker.io.run();
	spdlog::info("{} stopped", worker.name);

	try { if (_driver) _driver->threadEnd(); }
	catch (...) {}
}

// --------------------------------------
//    작업 투입 (어느 스레드에서나)
// --------------------------------------
void AuctionWorkerPool::Post(Worker& worker, Task task)
{
	Worker* w = &worker;

	// io_context의 큐는 FIFO 이므로, 같은 워커에 넣은 작업은 넣은 순서대로 실행된다.
	boost::asio::post(worker.io, [w, task = std::move(task)]() mutable
		{
			try
			{
				task(*w->conn);
			}
			catch (const sql::SQLException& e)
			{
				spdlog::error("[{}] SQL error {} : {}", w->name, e.getErrorCode(), e.what());
			}
			catch (const std::exception& e)
			{
				spdlog::error("[{}] task exception : {}", w->name, e.what());
			}
			catch (...)
			{
				spdlog::error("[{}] task unknown exception", w->name);
			}
		});
}

void AuctionWorkerPool::PostRead(Task task)
{
	if (_readers.empty())
	{
		spdlog::error("AuctionWorkerPool::PostRead called before Start");
		return;
	}

	const size_t index = _readRoundRobin.fetch_add(1, std::memory_order_relaxed) % _readers.size();
	Post(*_readers[index], std::move(task));
}

void AuctionWorkerPool::PostWrite(uint64 key, Task task)
{
	if (_writers.empty())
	{
		spdlog::error("AuctionWorkerPool::PostWrite called before Start");
		return;
	}

	// list_id / player_id 는 순차 증가 값이라 나머지 연산만으로 고르게 퍼진다.
	Post(*_writers[key % _writers.size()], std::move(task));
}