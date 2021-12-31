#ifndef BRILLIANTCLIENTSERVER_COMMON_H
#define BRILLIANTCLIENTSERVER_COMMON_H

#include <asio.hpp>
#include <asio/ssl.hpp>
#include "BrilliantLogger.h"

//better way to do this?
#define BCS_LOG Log<iClientServerLogId>

namespace Brilliant
{
	namespace ssl = asio::ssl;
	using tcp = asio::ip::tcp;

	static constexpr auto iClientServerLogId = std::numeric_limits<LoggerId>::max();
}

#endif