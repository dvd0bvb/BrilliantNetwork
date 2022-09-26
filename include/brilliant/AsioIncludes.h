#pragma once

#ifdef _WIN32
//header to include the correct windows version defines
//needed before asio.hpp
#include <sdkddkver.h>
#endif //_WIN32

//if standalone asio
#include <asio.hpp>
//else if boost.asio
//#inlcude <boost/asio.hpp>
// namespace Brilliant { namespace Network {
// namespace asio = boost::asio; //namespace alias to work with boost.asio
// } }
//endif