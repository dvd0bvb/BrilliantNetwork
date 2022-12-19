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

#endif