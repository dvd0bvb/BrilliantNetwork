#ifndef BRILLIANTCLIENTSERVER_COMMON_H
#define BRILLIANTCLIENTSERVER_COMMON_H

#include "BrilliantLogger.h"

//better way to do this?
#define BCS_LOG Log<iClientServerLogId>

namespace Brilliant
{
	static constexpr auto iClientServerLogId = std::numeric_limits<LoggerId>::max();
}

#endif