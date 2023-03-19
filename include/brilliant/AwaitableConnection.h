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
        template<class Protocol>
        class AwaitableConnection
        {        
        public:
            using protocol_type = Protocol;
            using socket_type = typename protocol_type::socket_type;

            AwaitableConnection(socket_type sock) : 
                socket(std::move(sock))
            {

            }

            asio::awaitable<error_code> Connect()
            {
                return impl.Connect(socket);
            }

            asio::awaitable<error_code> Connect(std::string_view host, std::string_view service)
            {
                auto result = co_await impl.Connect(socket, host, service);
                remote_endpoint = std::get<typename protocol_type::endpoint_type>(result);
                co_return std::get<error_code>(result);
            }

            error_code Disconnect()
            {
                return impl.Disconnect(socket);
            }

            bool IsConnected() const
            {
                return impl.IsConnected(socket);
            }

            asio::awaitable<std::pair<std::size_t, error_code>> Send(const asio::const_buffer& data)
            {
                if constexpr (is_datagram_protocol_v<typename protocol_type::protocol_type>)
                {
                    return impl.Send(socket, remote_endpoint, data);
                }
                else
                {
                    return impl.Send(socket, data);
                }
            }

            asio::awaitable<std::pair<std::size_t, error_code>> ReadInto(const asio::mutable_buffer& data)
            {
                if constexpr (is_datagram_protocol_v<typename protocol_type::protocol_type>)
                {
                    return impl.ReadInto(socket, remote_endpoint, data);
                }
                else
                {
                    return impl.ReadInto(socket, data);
                }
            }

        private:
            socket_type socket;
            typename protocol_type::endpoint_type remote_endpoint;
            protocol_type impl;
        };
    }
}