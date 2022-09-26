#pragma once

#ifdef _WIN32
#define BRILLIANT_NETWORK_DISABLE_WARN_4996 //lots of deprecated warnings from asio/ssl.hpp
#endif
//TODO: See if there need to be similar defines for posix

#ifdef BRILLIANT_NETWORK_DISABLE_WARN_4996
#pragma warning(disable : 4996)
#endif

//if standalone asio
#include <asio/ssl.hpp>
//else if boost.asio
//#inculde <boost/asio/ssl.hpp>
//endif

#ifdef BRILLIANT_NETWORK_DISABLE_WARN_4996
#pragma warning(default : 4996)
#endif