#include "pch.h"
#include "GameMember.h"

GameMember::GameMember()
{
	playerInfo = new Game::PlayerInfo();
}

GameMember::~GameMember()
{
	delete playerInfo;
}