/**
 * @file SocketTraits.h
 * @author David Brill (6david6brill6@gmail.com)
 * 
 * @copyright Copyright (c) 2023
 * Distributed under the Apache License 2.0 (see accompanying
 * file LICENSE or copy at http://www.apache.org/licenses/)
 * 
 * @brief Defines compile time traits for socket and protocol types
 */

#pragma once

#include <type_traits>

#include "AsioIncludes.h"
#include "Ssl.h"

namespace Brilliant
{
    namespace Network
    {
        //ssl checks
        template<class T> struct is_ssl_wrapped : std::false_type {};
        template<class T> struct is_ssl_wrapped<asio::ssl::stream<T>> : std::true_type {};

        template<class T>
        inline constexpr bool is_ssl_wrapped_v = is_ssl_wrapped<T>::value;

        //datagram protocol type check
        template<class T> struct is_datagram_protocol : std::false_type {};

        template<> struct is_datagram_protocol<asio::ip::udp> : std::true_type {};
        template<> struct is_datagram_protocol<asio::generic::datagram_protocol> : std::true_type {};
        
#ifdef BRILLIANT_NETWORK_HAS_LOCAL_SOCKETS
        template<> struct is_datagram_protocol<asio::local::datagram_protocol> : std::true_type {};
#endif

        template<class T>
        inline constexpr bool is_datagram_protocol_v = is_datagram_protocol<T>::value;

        //get protocol type from socket
        template<class Socket>
        struct socket_protocol_type
        {
            using type = typename Socket::protocol_type;
        };

        template<class Socket>
        struct socket_protocol_type<asio::ssl::stream<Socket>>
        {
            using type = typename Socket::lowest_layer_type::protocol_type;
        };

        template<class Socket>
        using socket_protocol_type_t = typename socket_protocol_type<Socket>::type;

        //local protocol type check
#ifdef BRILLIANT_NETWORK_HAS_LOCAL_SOCKETS
        template<class T> struct is_local_protocol : std::bool_constant<
            std::is_same_v<asio::local::stream_protocol, socket_protocol_type_t<typename T::socket>> ||
            std::is_same_v<asio::local::datagram_protocol, socket_protocol_type_t<typename T::socket>>> 
        {};
#else
        template<class T> struct is_local_protocol : std::false_type {};
#endif

        template<class Protocol>
        inline constexpr bool is_local_protocol_v = is_local_protocol<Protocol>::value;
    
        //stream type from boost.beast check
        template<class Socket> struct is_boost_beast_stream : std::false_type {};

#ifdef BRILLIANT_NETWORK_HAS_BOOST_BEAST
        template<class Protocol, class Executor, class RatePolicy>
        struct is_boost_beast_stream<boost::beast::basic_stream<Protocol, Executor, RatePolicy>> : std::true_type {};

        template<class Socket>
        struct is_boost_beast_stream<boost::beast::websocket::stream<Socket>> : std::true_type {};

        template<class Socket>
        struct is_boost_beast_stream<asio::ssl::stream<Socket>> : is_boost_beast_stream<Socket> {};
#endif

        template<class Socket>
        inline constexpr bool is_boost_beast_stream_v = is_boost_beast_stream<Socket>::value;

        //abstraction for getting lowest layer of a socket
        template<class Socket>
        requires (!is_boost_beast_stream_v<Socket>)
        auto& GetLowestLayer(Socket& socket)
        {
            return socket.lowest_layer();
        }

#ifdef BRILLIANT_NETWORK_HAS_BOOST_BEAST
        template<class Socket>
        requires (is_boost_beast_stream_v<Socket>)
        auto& GetLowestLayer(Socket& socket)
        {
            return boost::beast::get_lowest_layer(socket);
        }
#endif
    }
}