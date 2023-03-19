#pragma once

#ifndef BRILLIANT_NETWORK_NO_SSL

#ifdef _WIN32
#define BRILLIANT_NETWORK_DISABLE_WARN_4996 //lots of deprecated warnings from asio/ssl.hpp
#endif
//TODO: See if there need to be similar defines for posix

#ifdef BRILLIANT_NETWORK_DISABLE_WARN_4996
#pragma warning(disable : 4996)
#endif

#ifdef ASIO_STANDALONE
#include <asio/ssl.hpp>
#else
#include <boost/asio/ssl.hpp>
#endif

#ifdef BRILLIANT_NETWORK_DISABLE_WARN_4996
#pragma warning(default : 4996)
#endif

#endif //BRILLIANT_NETWORK_NO_SSL