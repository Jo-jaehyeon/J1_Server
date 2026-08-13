#include "pch.h"
#include "ObjectUtils.h"
#include "GameMember.h"

GameMemberPtr ObjectUtils::CreatePlayer(int id, std::string name)
{
	GameMemberPtr player = std::make_shared<GameMember>();
	player->playerInfo->set_name(name);
	player->playerInfo->set_player_id(id);


	return player;
}