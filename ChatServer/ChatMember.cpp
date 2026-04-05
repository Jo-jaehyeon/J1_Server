#include "pch.h"
#include "ChatMember.h"

ChatMember::ChatMember() 
{
	playerInfo = new Chat::ChatMemberInfo();
}

ChatMember::~ChatMember() 
{
	delete playerInfo;
}