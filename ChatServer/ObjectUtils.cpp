#include "pch.h"
#include "ObjectUtils.h"
#include "ChatMember.h"

std::atomic<unsigned int> ObjectUtils::s_idGenerator = 1;

uint32 ObjectUtils::GenerateId()
{
	const uint32 newId = s_idGenerator.fetch_add(1);
	return newId;
}

ChatMemberPtr ObjectUtils::CreatePlayer(std::string name)
{
	ChatMemberPtr player = std::make_shared<ChatMember>();
	player->playerInfo->set_name(name);
	player->playerInfo->set_player_id(GenerateId());

	return player;
}
