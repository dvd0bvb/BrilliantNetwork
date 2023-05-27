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
            using socket_type = typename protocol_type::socket_type;
            using connection_type = AwaitableConnection<protocol_type>;

            AwaitableClient(asio::any_io_executor executor) : 
                connection(connection_type(socket_type{ executor }))
            {

            }

            AwaitableClient(asio::any_io_executor executor, asio::ssl::context& ssl) : 
                connection(connection_type(socket_type{ executor, ssl }))
            {

            }

            ~AwaitableClient()
            {
                Disconnect();
            }

            //return the awaitable object that connection creates
            auto Connect(std::string_view host, std::string_view service)
            {
                return connection.Connect(host, service);
            }

            error_code Disconnect()
            {
                return connection.Disconnect();
            }

            template<class T>
            auto Send(T&& msg)
            {
                return connection.Send(std::forward<T>(msg));
            }

            template<class T>
            auto Read(T&& msg)
            {
                return connection.ReadInto(std::forward<T>(msg));
            }

        private:
            connection_type connection;
        };
    }
}