#pragma once

class ObjectUtils
{
public:
	static GameMemberPtr CreatePlayer(int id, std::string name);

private:
	static std::atomic<uint32> s_idGenerator;
};