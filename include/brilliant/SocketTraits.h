#pragma once

#include <type_traits>

#include "AsioIncludes.h"
#include "Ssl.h"

namespace Brilliant
{
    namespace Network
    {
        template<class T> struct is_ssl_wrapped : std::false_type {};
        template<class T> struct is_ssl_wrapped<asio::ssl::stream<T>> : std::true_type {};

        template<class T>
        inline constexpr bool is_ssl_wrapped_v = is_ssl_wrapped<T>::value;

        template<class T> struct is_datagram_protocol : std::false_type {};

        template<> struct is_datagram_protocol<asio::ip::udp> : std::true_type {};
        template<> struct is_datagram_protocol<asio::generic::datagram_protocol> : std::true_type {};
        //will this need to change for boost.asio?
#ifdef ASIO_HAS_LOCAL_SOCKETS
        template<> struct is_datagram_protocol<asio::local::datagram_protocol> : std::true_type {};
#endif

        template<class T>
        inline constexpr bool is_datagram_protocol_v = is_datagram_protocol<T>::value;

        template<class Socket>
        struct socket_protocol_type
        {
            using type = Socket::protocol_type;
        };

        template<class Socket>
        struct socket_protocol_type<asio::ssl::stream<Socket>>
        {
            using type = Socket::lowest_layer_type::protocol_type;
        };

        template<class Socket>
        using socket_protocol_type_t = socket_protocol_type<Socket>::type;

#ifdef ASIO_HAS_LOCAL_SOCKETS
        template<class T> struct is_local_protocol : std::bool_constant<
            std::is_same_v<asio::local::stream_protocol, socket_protocol_type_t<typename T::socket>> ||
            std::is_same_v<asio::local::datagram_protocol, socket_protocol_type_t<typename T::socket>>> 
        {};
#else
        template<class T> struct is_local_protocol : std::false_type {};
#endif

        template<class Protocol>
        inline constexpr bool is_local_protocol_v = is_local_protocol<Protocol>::value;
    }
}