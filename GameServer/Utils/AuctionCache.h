#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <boost/asio/steady_timer.hpp>
#include "Network/Protocol/SS_AuctionProtocol.pb.h"

class GameSession;
using Clock = std::chrono::steady_clock;

enum class Verdict
{
	Pass,            // 통과 -> AuctionServer로 전달
	Unknown,         // 판단 불가(캐시 없음/오래됨/범위 밖) -> 그냥 전달
	RejectNotFound,  // 캐시에 없음 (팔렸거나 만료)
	RejectShortage,  // 요청 수량 > 남은 수량
	RejectOwn,       // 본인 매물
};

struct Snapshot
{
	std::vector<Game::AuctionItemInfo>        items;          // list_id 오름차순
	std::unordered_map<int64, size_t>         index;          // list_id -> items 인덱스
	std::shared_ptr<const BufferPooledVector> listPacket;     // 클라이언트 응답(헤더 포함). null이면 사용 불가
	int64                                     maxListId = 0;  // 스냅샷에 포함된 가장 큰 list_id
	bool                                      complete = true;// kMaxItems에 걸려 잘렸으면 false
	Clock::time_point                         fetchedAt;
};

class AuctionCache
{
public:
	AuctionCache(const AuctionCache&) = delete;
	AuctionCache& operator=(const AuctionCache&) = delete;

	static AuctionCache& Instance();
	void Init(asio::io_context& auctionIo);

	void RequestRefresh();
	void OnRefreshResponse(SS_Auction::RES_AUCTION_LIST& pkt);
	void MarkDirty();

	bool TrySendList(Session& session);
	Verdict ValidatePurchase(int64 listId, int64 count, int64 buyerId);

private:
	AuctionCache() = default;

	// ---- 스냅샷 포인터 접근 (RW 락) ----
	std::shared_ptr<const Snapshot> LoadSnapshot() const;
	void StoreSnapshot(std::shared_ptr<const Snapshot> s);

	bool IsFresh(const Snapshot& s) const;
	void FinishRefresh(SS_Auction::RES_AUCTION_LIST& pkt);
	void OnRequestTimeout();
	void BuildClientPacket(Snapshot& s);						// 미리 직렬화 된 리스트 준비
	void ArmTimer(Clock::duration delay);						// 타이머 예약
	void OnTimer(const boost::system::error_code& ec);			// 타이머 실행

public:
	// ---- 정책 상수 ----
	static constexpr int64  kCacheSessionId = 0;               // 캐시 갱신용 예약 ID (GameSessionManager는 1부터 발급)
	static constexpr size_t kClientListMax = 100;             // 클라이언트에 내려주는 최대 건수 (AuctionServer LIMIT과 맞춘다)
	static constexpr auto   kRefreshInterval = std::chrono::seconds(60);
	static constexpr auto   kMinRefreshGap = std::chrono::seconds(3);    // 조기 갱신 최소 간격
	static constexpr auto   kRequestTimeout = std::chrono::seconds(5);
	static constexpr auto   kStaleLimit = std::chrono::minutes(3);

private:
	// 공유 상태: 이 락이 지키는 것은 _snapshot 포인터뿐
	mutable std::shared_mutex            _snapshotMutex;
	std::shared_ptr<const Snapshot>      _snapshot;

	// ---- 경매장 스레드 전용 상태 ----
	bool                                 _inFlight = false;
	int32                                _refreshSeq = 0;    // request_id로 사용 -> 타임아웃 후 늦게 온 응답 구분

	bool                                 _dirty = false;
	Clock::time_point                    _lastRequestAt{};

	std::unique_ptr<asio::steady_timer>  _timer;             // 다음 갱신 예약
	std::unique_ptr<asio::steady_timer>  _timeoutTimer;      // 페이지 응답 타임아웃
	bool                                 _timerArmed = false;
	Clock::time_point                    _timerTarget{};

	// 통계 (모니터링용, 순서 보장 불필요)
	std::atomic<uint64>                  _hit{ 0 };
	std::atomic<uint64>                  _miss{ 0 };
	std::atomic<uint64>                  _rejected{ 0 };
};

