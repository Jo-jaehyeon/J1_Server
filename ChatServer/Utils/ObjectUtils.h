#pragma once

class ObjectUtils
{
public:
	static uint32 GenerateId();

	static ChatMemberPtr CreatePlayer(std::string name);

private:
	static std::atomic<uint32> s_idGenerator;
};

