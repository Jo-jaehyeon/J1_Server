#pragma once

class GameSession;
class GameRoom;

class GameMember : public std::enable_shared_from_this<GameMember>
{
public:
	GameMember();
	virtual ~GameMember();

public:
	Game::GameMemberInfo* playerInfo;
	std::weak_ptr<GameSession> session;

public:
	std::atomic<std::weak_ptr<GameRoom>> room;
};
