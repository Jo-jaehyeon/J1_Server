#pragma once
#include "ChatProtocol.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum
{
};

// Custom Handler
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);


class { { output } }
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
	}
	static bool HandlePacket(PacketSessionRef & session, BYTE * buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}


