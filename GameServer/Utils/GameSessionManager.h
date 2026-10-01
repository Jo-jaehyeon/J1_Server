#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

class GameSession;

class GameSessionManager
{
public:
    static GameSessionManager& Instance();

    GameSessionManager(const GameSessionManager&) = delete;
    GameSessionManager& operator=(const GameSessionManager&) = delete;

    // 새로 접속한 세션을 등록하고 고유 SessionId를 발급해 반환합니다.
    // 내부적으로 session->SetId(id)를 호출해 세션 스스로도 자신의 ID를 알 수 있게 합니다.
    uint64 Register(const GameSessionPtr& session);

    // sessionId로 세션을 조회합니다. 이미 끊긴 세션이면 nullptr을 반환합니다.
    std::shared_ptr<GameSession> Find(uint64 sessionId);

    // 세션 연결이 끊어졌을 때 호출해 등록을 해제합니다.
    void Unregister(uint64 sessionId);

    // 현재 등록되어 있는(살아있는 것으로 추정되는) 세션 수. 참고/모니터링용입니다.
    size_t Count();

private:
    GameSessionManager() = default;

    std::mutex mutex_;
    std::unordered_map<uint64, std::weak_ptr<GameSession>> sessions_;
    std::atomic<uint64> nextId_{ 1 };
};