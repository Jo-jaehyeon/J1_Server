#pragma once
#include "../Protocol/ChatProtocol.pb.h"
#include "Packet.h"

using PacketHandlerFunc = std::function<bool(SessionPtr&, boost::asio::mutable_buffer&, int32&)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum
{
{%- for pkt in parser.total_pkt %}
	PKT_{{pkt.name}} = {{pkt.id}},
{%- endfor %}
};

// Custom Handler
bool Handle_INVALID(SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset);

{%- for pkt in parser.recv_pkt %}
bool Handle_{{pkt.name}}(SessionPtr& session, Chat::{{pkt.name}}& pkt);
{%- endfor %}


class {{output}}
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;


		{%- for pkt in parser.recv_pkt %}
		GPacketHandler[PKT_{{pkt.name}}] = [](SessionPtr& session, boost::asio::mutable_buffer& buffer, int32& offset) {
			return HandlePacket < Chat::{{pkt.name}} > (Handle_{{pkt.name}}, session, buffer, offset);
			};
		{%- endfor %}
	}

	static bool HandlePacket(SessionPtr & session, const PacketHeader& header, char* ptr, size_t size)
	{
		boost::asio::mutable_buffer buffer = boost::asio::buffer(ptr, size);
		int offset = 4;

		return GPacketHandler[header.Code](session, buffer, offset);
	}

private:
	template<typename PacketType, typename ProcessFunc>
	static bool DispatchPacket(ProcessFunc func, SessionPtr & session, boost::asio::mutable_buffer& buffer, int32& offset)
	{
		PacketType pkt;
		if (!PacketUtil::Parse(pkt, buffer, buffer.size(), offset))
		{
			spdlog::error("Failed to Handle Packet : {}", pkt);
			return false;
		}

		return func(session, pkt);
	}
};
