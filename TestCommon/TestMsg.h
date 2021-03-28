#pragma once

#include <stdint.h>

namespace
{
	enum class TestMessage : std::uint32_t
	{
		ServerAccept,
		ServerDeny,
		ServerPing,
		MessageAll,
		ServerMessage
	};
}