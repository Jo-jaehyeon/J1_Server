#include "pch.h"
#include "AuctionCache.h"
#include "Session.h"
#include "AuctionSession.h"
#include "Network/Protocol/GameEnum.pb.h"
#include "Network/Protocol/AuctionProtocol.pb.h"

AuctionCache& AuctionCache::Instance()
{
	static AuctionCache instance;
	return instance;
}

void AuctionCache::Init(asio::io_context& auctionIo)
{
	_timer = std::make_unique<asio::steady_timer>(auctionIo);
	_timeoutTimer = std::make_unique<asio::steady_timer>(auctionIo);
}

// ------------------------------------------------------------
//  스냅샷 포인터 (RW 락) - 락 안에서는 포인터 복사/교체만 한다
// ------------------------------------------------------------
std::shared_ptr<const Snapshot> AuctionCache::LoadSnapshot() const
{
	std::shared_lock lock(_snapshotMutex);
	return _snapshot;                                  // 포인터 복사 + 참조 카운트 증가뿐
}

void AuctionCache::StoreSnapshot(std::shared_ptr<const Snapshot> s)
{
	std::shared_ptr<const Snapshot> old;
	{
		std::unique_lock lock(_snapshotMutex);
		old = std::move(_snapshot);
		_snapshot = std::move(s);
	}
	// old는 여기서 해제된다. (큰 객체의 소멸이 락 안에서 일어나지 않도록 밖으로 뺐다)
}

bool AuctionCache::IsFresh(const Snapshot& s) const
{
	return (Clock::now() - s.fetchedAt) < kStaleLimit;
}

// ------------------------------------------------------------
//							갱신 
// ------------------------------------------------------------
void AuctionCache::RequestRefresh()
{
	if (_inFlight || !GAuctionSession || !_timer)
		return;

	_inFlight = true;
	++_refreshSeq;
	_lastRequestAt = Clock::now();

	SS_Auction::REQ_AUCTION_LIST req;
	req.set_session_id(kCacheSessionId);
	req.set_request_id(_refreshSeq);
	req.set_mylist(false);

	GAuctionSession->SendPacket(req, SS_Auction::PacketType::PKT_REQ_AUCTION_LIST);

	_timeoutTimer->expires_after(kRequestTimeout);
	_timeoutTimer->async_wait([this, seq = _refreshSeq](const boost::system::error_code& ec)
		{
			if (ec) return;
			if (_inFlight && seq == _refreshSeq)	OnRequestTimeout();
		});
}

void AuctionCache::OnRefreshResponse(SS_Auction::RES_AUCTION_LIST& pkt)
{
	if (!_inFlight || pkt.request_id() != _refreshSeq)	return;
	FinishRefresh(pkt);
}


void AuctionCache::FinishRefresh(SS_Auction::RES_AUCTION_LIST& pkt)
{
	auto snap = std::make_shared<Snapshot>();
	snap->fetchedAt = Clock::now();

	snap->items.reserve(pkt.listinfo_size());
	for (auto& item : *pkt.mutable_listinfo())
		snap->items.push_back(std::move(item));

	snap->index.reserve(snap->items.size());
	for (size_t i = 0; i < snap->items.size(); i++)
	{
		const int64 id = snap->items[i].list_id();
		snap->index.emplace(id, i);
		if (id > snap->maxListId)
			snap->maxListId = id;
	}

	BuildClientPacket(*snap);

	const size_t itemCount = snap->items.size();

	// 교체는 락 안에서
	StoreSnapshot(std::move(snap));

	_inFlight = false;
	_timeoutTimer->cancel();

	spdlog::info("AuctionCache refreshed : {} items (hit {}, miss {}, rejected {})", itemCount, _hit.load(), _miss.load(), _rejected.load());

	// dirty면 최소 간격만 지키고 빨리, 아니면 정규 주기로
	if (_dirty)
	{
		_dirty = false;
		const auto elapsed = Clock::now() - _lastRequestAt;
		ArmTimer(elapsed >= kMinRefreshGap ? Clock::duration::zero() : kMinRefreshGap - elapsed);
	}
	else
	{
		ArmTimer(kRefreshInterval);
	}
}

void AuctionCache::OnRequestTimeout()
{
	spdlog::warn("AuctionCache refresh timeout(seq {})", _refreshSeq);
	_inFlight = false;
	ArmTimer(kRefreshInterval);
}

void AuctionCache::BuildClientPacket(Snapshot& s)
{
	// 보낼 때마다 직렬화 하는 과정을 막기 위해 미리 직렬화해서 캐싱
	Game::RES_AUCTION_LIST pkt;
	pkt.set_mylist(false);

	const size_t n = std::min(s.items.size(), kClientListMax);
	for (size_t i = 0; i < n; i++)
		*pkt.add_itemlist() = s.items[i];

	if (pkt.ByteSizeLong() > static_cast<size_t>(SHRT_MAX))
	{
		spdlog::error("AuctionCache client packet too large : {}", pkt.ByteSizeLong());
		return;			// listPacket null -> TrySendList가 false 반환
	}

	const size_t size = PacketUtil::RequiredSize(pkt);
	auto buf = std::make_shared<BufferPooledVector>(size);

	// 이 버퍼를 모든 클라이언트 전송이 공유한다 (복사 없음)
	if (PacketUtil::Serialize(asio::buffer(*buf), static_cast<short>(Game::PacketType::PKT_RES_AUCTION_LIST), pkt))
		s.listPacket = std::move(buf);         
}

void AuctionCache::MarkDirty()
{
	_dirty = true;
	if (_inFlight) return;

	const auto elapsed = Clock::now() - _lastRequestAt;
	ArmTimer(elapsed >= kMinRefreshGap ? Clock::duration::zero() : kMinRefreshGap - elapsed);
}

void AuctionCache::ArmTimer(Clock::duration delay)
{
	const auto target = Clock::now() + delay;
	if (_timerArmed && target >= _timerTarget)
		return;                                // 이미 더 이른 예약이 있음

	_timerArmed = true;
	_timerTarget = target;
	_timer->expires_at(target);                // 기존 대기는 operation_aborted로 끝난다
	_timer->async_wait([this](const boost::system::error_code& ec) { OnTimer(ec); });
}

void AuctionCache::OnTimer(const boost::system::error_code& ec)
{
	if (ec) return;                            // 재예약으로 취소된 대기
	_timerArmed = false;
	_dirty = false;
	RequestRefresh();

	if (!_inFlight)                            // 연결 전이라 요청을 못 보낸 경우 -> 다음 주기에 재시도
		ArmTimer(kRefreshInterval);
}

bool AuctionCache::TrySendList(Session& session)
{
	const auto snap = LoadSnapshot();          // 이 호출 동안 한 버전만 본다
	if (!snap || !snap->listPacket || !IsFresh(*snap))
	{
		_miss.fetch_add(1, std::memory_order_relaxed);
		return false;
	}

	session.SendShared(snap->listPacket);      // 같은 버퍼를 공유해서 큐에 넣는다 (복사 없음)
	_hit.fetch_add(1, std::memory_order_relaxed);
	return true;
}

Verdict AuctionCache::ValidatePurchase(int64 listId, int64 count, int64 buyerId)
{
	const auto snap = LoadSnapshot();
	if (!snap || !IsFresh(*snap))
		return Verdict::Unknown;

	auto it = snap->index.find(listId);
	if (it == snap->index.end())
	{
		// 캐시는 "list_id 오름차순 앞에서 N건"이다.
		//  - maxListId보다 크면: 캐시 범위 밖(스냅샷 이후 등록 / 100건 밖) -> 모른다 -> 전달
		//  - maxListId 이하인데 없으면: 그 사이 팔렸거나 만료된 것 -> 확정 거절
		if (listId > snap->maxListId)
			return Verdict::Unknown;
		_rejected.fetch_add(1, std::memory_order_relaxed);
		return Verdict::RejectNotFound;
	}

	const Game::AuctionItemInfo& item = snap->items[it->second];
	if (count <= 0 || count > item.count())
	{
		_rejected.fetch_add(1, std::memory_order_relaxed);
		return Verdict::RejectShortage;
	}
	if (item.player_id() == buyerId)           // player_id는 판매자 ID
	{
		_rejected.fetch_add(1, std::memory_order_relaxed);
		return Verdict::RejectOwn;
	}

	return Verdict::Pass;
}
