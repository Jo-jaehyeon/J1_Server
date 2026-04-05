#pragma once

#include <memory>

#include "Packet.h"

#define USING_SHARED_PTR(name) using name##Ptr = std::shared_ptr<class name>;

USING_SHARED_PTR(ChatSession)
USING_SHARED_PTR(ChatServer)
USING_SHARED_PTR(ChatRoom)
USING_SHARED_PTR(ChatMember)