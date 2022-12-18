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
                connection(socket_type{context})
            {

            }

            AwaitableClient(asio::io_context& context, asio::ssl::context& ssl) : 
                connection(asio::ssl::stream<socket_type>{context, ssl})
            {

            }

            ~AwaitableClient()
            {
                Disconnect();
            }

            //need trailing return types for coroutines
            auto Connect(std::string_view host, std::string_view service)
            {
                return connection.Connect(host, service);
            }

            void Disconnect()
            {
                connection.Disconnect();
            }

            template<class T>
            auto Send(T&& msg)
            {
                return ::Brilliant::Network::Send(connection, std::forward<T>(msg));
            }

            template<class T>
            auto Read(T&& msg)
            {
                return ReadInto(connection, std::forward<T>(msg));
            }

        private:
            connection_type connection;
        };
    }
}