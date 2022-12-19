#pragma once

#include <ranges>

#include "AwaitableConnection.h"

namespace Brilliant
{
    namespace Network
    {
        template<class Protocol>
        class AwaitableClient
        {
        public:
            using protocol_type = Protocol;
            using socket_type = typename protocol_type::socket;
            using connection_type = AwaitableConnection<socket_type>;

            AwaitableClient(asio::io_context& context) : 
                connection(std::make_unique<AwaitableConnection<socket_type>>(socket_type{ context }))
            {

            }

            AwaitableClient(asio::io_context& context, asio::ssl::context& ssl) : 
                connection(std::make_unique<AwaitableConnection<asio::ssl::stream<socket_type>>>(asio::ssl::stream<socket_type>{ context, ssl }))
            {

            }

            ~AwaitableClient()
            {
                Disconnect();
            }

            //return the awaitable object that connection creates
            auto Connect(std::string_view host, std::string_view service)
            {
                return connection->Connect(host, service);
            }

            void Disconnect()
            {
                connection->Disconnect();
            }

            template<class T>
            auto Send(T&& msg)
            {
                return connection->Send(std::forward<T>(msg));
            }

            template<class T>
            auto Read(T&& msg)
            {
                return connection->ReadInto(std::forward<T>(msg));
            }

        private:
            std::unique_ptr<AwaitableConnectionInterface> connection;
        };
    }
}