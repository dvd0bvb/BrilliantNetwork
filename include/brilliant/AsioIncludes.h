/**
 * @file AsioIncludes.h
 * @author David Brill (6david6brill6@gmail.com)
 * 
 * @copyright Copyright (c) 2023
 * Distributed under the Apache License 2.0 (see accompanying
 * file LICENSE or copy at http://www.apache.org/licenses/)
 * 
 * @brief Provides asio includes and some asio type aliases for internal
 * use of the Brilliant::Network library
 */

#pragma once

/**
 * @namespace Brilliant
 * @brief Provides classes, functions and libraries in the Brilliant namespace
 */
namespace Brilliant 
{
    /**
     * @namespace Network
     * @brief Provides classes that make up the Brilliant::Network library
     */
    namespace Network
    {

    }
}

#ifdef _WIN32
//header to include the correct windows version defines
//needed before asio.hpp
#include <sdkddkver.h>
#endif //_WIN32

#ifdef ASIO_STANDALONE

#include <asio.hpp>
#include <asio/experimental/coro.hpp>
#include <asio/experimental/use_coro.hpp>

//just in case
#define BRILLIANT_NETWORK_NO_BOOST_BEAST
#undef BRILLIANT_NETWORK_HAS_BOOST_BEAST

#ifdef ASIO_HAS_LOCAL_SOCKETS
#define BRILLIANT_NETWORK_HAS_LOCAL_SOCKETS
#endif //ASIO_HAS_LOCAL_SOCKETS

namespace Brilliant
{
    namespace Network
    {
        using error_code = asio::error_code;
    }
}

#else //ASIO_STANDALONE

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/experimental/use_coro.hpp>

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
#define BRILLIANT_NETWORK_HAS_LOCAL_SOCKETS
#endif //BOOST_ASIO_HAS_LOCAL_SOCKETS

#ifndef BRILLIANT_NETWORK_NO_BOOST_BEAST
#define BRILLIANT_NETWORK_HAS_BOOST_BEAST
#endif //BRILLIANT_NETWORK_NO_BOOST_BEAST

#ifdef BRILLIANT_NETWORK_HAS_BOOST_BEAST
#include <boost/beast.hpp>
#endif //BRILLIANT_NETWORK_HAS_BOOST_BEAST

namespace Brilliant 
{ 
    namespace Network 
    {
        namespace asio = boost::asio; //namespace alias to work with boost.asio
        using error_code = boost::system::error_code; //name alias for error code
    } 
}

#endif //ASIO_STANDALONE