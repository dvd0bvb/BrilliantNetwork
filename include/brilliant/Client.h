#pragma once

#include <optional>
#include <memory>
#include <string_view>
#include <ranges>

#include "ConnectionProcess.h"
#include "Connection.h"
#include "Ssl.h"

namespace Brilliant
{
    namespace Network
    {
        template<class Protocol>
        class Client
        {
        public:
            using protocol_type = Protocol;
            using socket_type = typename protocol_type::socket;

            //TODO: allow customization of allocators for connection
            Client(asio::io_context& ctx, ConnectionProcess& proc) : 
                connection(Connection<socket_type>::Create(socket_type{ctx}, proc))
            {

            }

            Client(asio::io_context& ctx, asio::ssl::context& ssl, ConnectionProcess& proc) : 
                connection(Connection<asio::ssl::stream<socket_type>>::Create({ ctx, ssl }, proc))
            {

            }

            ~Client()
            {
                Disconnect();
            }

            void Connect(std::string_view host, std::string_view service)
            {
                connection->Connect(host, service);
            }

            void Disconnect()
            {
                connection->Disconnect();
            }

            template<class T>
            void Send(T&& t)
            {
                connection->Send(std::forward<T>(t));
            }

        private:                        
            std::shared_ptr<ConnectionInterface> connection;
        };
    }
}