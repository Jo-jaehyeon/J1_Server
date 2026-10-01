#include "pch.h"
#include "GameSessionManager.h"

#include "GameSession.h"

GameSessionManager& GameSessionManager::Instance()
{
    static GameSessionManager instance;
    return instance;
}

uint64 GameSessionManager::Register(const GameSessionPtr& session)
{
    const uint64 id = nextId_.fetch_add(1, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[id] = session;
    }

    session->SetSessionId(id);

    return id;
}

std::shared_ptr<GameSession> GameSessionManager::Find(uint64 sessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(sessionId);
    if (it == sessions_.end())
    {
        return nullptr;
    }

    // weak_ptr이 이미 만료되었으면(클라이언트가 끊겼으면) lock()이 nullptr을 반환합니다.
    return it->second.lock();
}

void GameSessionManager::Unregister(uint64 sessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(sessionId);
}

size_t GameSessionManager::Count()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}