#pragma once

#include <ranges>

#include "AsioIncludes.h"
#include "AwaitableConnectionInterface.h"
#include "SocketTraits.h"
#include "EndpointHelper.h"

namespace Brilliant
{
    namespace Network
    {
        template<class Socket>
        class AwaitableConnection : public AwaitableConnectionInterface
        {        
        public:
            using socket_type = Socket;
            using protocol_type = socket_protocol_type_t<socket_type>;

            AwaitableConnection(Socket sock) : socket(std::move(sock))
            {

            }

            asio::awaitable<asio::error_code> Connect()
            {
                asio::error_code ec;

                if constexpr (is_ssl_wrapped_v<socket_type>)
                {
                    co_await socket.async_handshake(socket.server, asio::redirect_error(asio::use_awaitable, ec));
                    if (ec)
                    {
                        co_return ec;
                    }
                }

                co_return ec;
            }

            asio::awaitable<asio::error_code> Connect(std::string_view host, std::string_view service)
            {
                asio::error_code ec;
                auto results = co_await ResolveEndpoints<protocol_type>(socket.get_executor(), host, service, ec);

                if (ec) { co_return ec; }

                remote_endpoint = co_await asio::async_connect(socket.lowest_layer(), results, asio::redirect_error(asio::use_awaitable, ec));

                if (ec) { co_return ec; }

                if constexpr (is_ssl_wrapped_v<socket_type>)
                {
                    co_await socket.async_handshake(socket.client, asio::redirect_error(asio::use_awaitable, ec));
                    if (ec) { co_return ec; }
                }

                co_return ec;
            }

            asio::error_code Disconnect()
            {
                asio::error_code ec;
                socket.lowest_layer().cancel(ec);
                if (ec) { return ec; }

                if (IsConnected())
                {
                    if constexpr (is_ssl_wrapped_v<socket_type>)
                    {
                        socket.shutdown(ec);
                        if (ec) { return ec; }
                    }

                    socket.lowest_layer().shutdown(socket_type::lowest_layer_type::shutdown_both, ec);
                    if (ec) { return ec; }

                    socket.lowest_layer().close(ec);
                    if (ec) { return ec; }
                }

                return ec;
            }

            bool IsConnected() const
            {
                return socket.lowest_layer().is_open();
            }

            asio::awaitable<std::pair<std::size_t, asio::error_code>> Send(const asio::const_buffer& data)
            {
                asio::error_code ec;
                std::size_t bytes_written = 0;

                if constexpr (is_datagram_protocol_v<protocol_type>)
                {
                    bytes_written = co_await socket.async_send_to(data, remote_endpoint, asio::redirect_error(asio::use_awaitable, ec));
                }
                else
                {
                    bytes_written = co_await asio::async_write(socket, data, asio::redirect_error(asio::use_awaitable, ec));
                }

                co_return std::make_pair(bytes_written, ec);
            }

            asio::awaitable<std::pair<std::size_t, asio::error_code>> ReadInto(const asio::mutable_buffer& data)
            {
                asio::error_code ec;
                std::size_t bytes_read = 0;

                if constexpr (is_datagram_protocol_v<protocol_type>)
                {
                    bytes_read = co_await socket.async_receive_from(data, remote_endpoint, asio::redirect_error(asio::use_awaitable, ec));
                }
                else
                {
                    bytes_read = co_await asio::async_read(socket, data, asio::redirect_error(asio::use_awaitable, ec));
                }

                co_return std::make_pair(bytes_read, ec);
            }

        private:
            socket_type socket;
            typename protocol_type::endpoint remote_endpoint;
        };
    }
}