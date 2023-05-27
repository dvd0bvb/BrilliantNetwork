#pragma once

#include "AsioIncludes.h"
#include "SocketTraits.h"
#include "EndpointHelper.h"

namespace Brilliant
{
    namespace Network
    {
        template<class Protocol, bool UseSsl = false>
        struct BasicProtocol
        {       
            using protocol_type = Protocol;
            using socket_type = std::conditional_t<UseSsl, asio::ssl::stream<typename protocol_type::socket>, typename protocol_type::socket>;
            using resolver_type = asio::ip::basic_resolver<protocol_type>;
            using endpoint_type = typename protocol_type::endpoint;
            using acceptor_type = asio::basic_socket_acceptor<protocol_type>;

            asio::awaitable<error_code> Connect(socket_type& socket)
            {
                if constexpr (UseSsl)
                {
                    error_code ec{};
                    co_await socket.async_handshake(socket.server, asio::redirect_error(asio::use_awaitable, ec));
                    co_return ec;
                }

                co_return error_code{};
            }

            asio::awaitable<std::pair<endpoint_type, error_code>> Connect(socket_type& socket, std::string_view host, std::string_view service)
            {
                error_code ec{};
                auto results = co_await ResolveEndpoints<BasicProtocol<protocol_type>>(co_await asio::this_coro::executor, host, service, ec);
                
                if (ec) { co_return std::make_pair(endpoint_type{}, ec); }

                auto ep = co_await asio::async_connect(socket.lowest_layer(), results, asio::redirect_error(asio::use_awaitable, ec));
                if (ec) { co_return std::make_pair(endpoint_type{}, ec); }

                if constexpr (UseSsl)
                {
                    co_await socket.async_handshake(socket.client, asio::redirect_error(asio::use_awaitable, ec));
                    if (ec) { co_return std::make_pair(endpoint_type{}, ec); }
                }

                co_return std::make_pair(ep, ec);;
            }

            error_code Disconnect(socket_type& socket)
            {
                error_code ec{};
                auto& lowest_layer = socket.lowest_layer();

                lowest_layer.cancel(ec);
                if (ec) { return ec; }

                if (IsConnected(socket))
                {
                    if constexpr (UseSsl)
                    {
                        socket.shutdown(ec);
                        if (ec) { return ec; }
                    }

                    lowest_layer.shutdown(protocol_type::socket::shutdown_both, ec);
                    if (ec) { return ec; }

                    lowest_layer.close(ec);
                }

                return ec;
            }

            bool IsConnected(const socket_type& socket) const
            {
                return socket.lowest_layer().is_open();
            }

            asio::awaitable<std::pair<std::size_t, error_code>> Send(socket_type& socket, const asio::const_buffer& data)
                requires(!is_datagram_protocol_v<protocol_type>)
            {
                error_code ec{};
                const std::size_t bytes_written = co_await asio::async_write(socket, data, asio::redirect_error(asio::use_awaitable, ec));
                co_return std::make_pair(bytes_written, ec);
            }

            asio::awaitable<std::pair<std::size_t, error_code>> Send(socket_type& socket, const endpoint_type& destination, const asio::const_buffer& data)
                requires (is_datagram_protocol_v<protocol_type>)
            {
                error_code ec{};
                const std::size_t bytes_written = co_await socket.async_send_to(data, destination, asio::redirect_error(asio::use_awaitable, ec));
                co_return std::make_pair(bytes_written, ec);
            }

            asio::awaitable<std::pair<std::size_t, error_code>> ReadInto(socket_type& socket, const asio::mutable_buffer& data)
                requires (!is_datagram_protocol_v<protocol_type>)
            {
                error_code ec{};
                const std::size_t bytes_read = co_await asio::async_read(socket, data, asio::redirect_error(asio::use_awaitable, ec));
                co_return std::make_pair(bytes_read, ec);
            }

            asio::awaitable<std::pair<std::size_t, error_code>> ReadInto(socket_type& socket, endpoint_type& destination, const asio::mutable_buffer& data)
                requires (is_datagram_protocol_v<protocol_type>)
            {
                error_code ec{};
                const std::size_t bytes_read = co_await socket.async_receive_from(data, destination, asio::redirect_error(asio::use_awaitable, ec));
                co_return std::make_pair(bytes_read, ec);
            }

            static asio::experimental::coro<void, error_code> Accept(acceptor_type& acceptor, socket_type& socket)
            {
                co_await acceptor.async_accept(socket, asio::experimental::use_coro);
                co_return error_code{};
            }
        };

        using TcpProtocol = BasicProtocol<asio::ip::tcp>;
        using UdpProtocol = BasicProtocol<asio::ip::udp>;
        //using Icmp = BasicProtocol<asio::ip::icmp>; //TODO: test this
#ifdef BRILLIANT_NETWORK_HAS_LOCAL_SOCKETS
        using LocalStreamProtocol = BasicProtocol<asio::local::stream_protocol>;
        using LocalDatagramProtocol = BasicProtocol<asio::local::datagram_protocol>;
#endif //BRILLIANT_NETWORK_HAS_LOCAL_SOCKETS

        using SslProtocol = BasicProtocol<asio::ip::tcp, true>;
    }
}