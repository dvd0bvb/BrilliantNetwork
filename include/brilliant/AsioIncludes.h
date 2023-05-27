#pragma once

#ifdef _WIN32
//header to include the correct windows version defines
//needed before asio.hpp
#include <sdkddkver.h>
#endif //_WIN32

#ifdef ASIO_STANDALONE

#include <asio.hpp>
#include <asio/experimental/coro.hpp>
#include <asio/experimental/use_coro.hpp>

namespace Brilliant
{
    namespace Network
    {
        using error_code = asio::error_code;
    }
}

#ifdef ASIO_HAS_LOCAL_SOCKETS
#define BRILLIANT_NETWORK_HAS_LOCAL_SOCKETS
#endif

#else

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/experimental/use_coro.hpp>

namespace Brilliant 
{ 
    namespace Network 
    {
        namespace asio = boost::asio; //namespace alias to work with boost.asio
        using error_code = boost::system::error_code; //name alias for error code
    } 
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
#define BRILLIANT_NETWORK_HAS_LOCAL_SOCKETS
#endif

#ifndef BRILLIANT_NETWORK_NO_BOOST_BEAST
#define BRILLIANT_NETWORK_HAS_BOOST_BEAST
#endif

#ifdef BRILLIANT_NETWORK_HAS_BOOST_BEAST
#include <boost/beast.hpp>
#endif

#endif