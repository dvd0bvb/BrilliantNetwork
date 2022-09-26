#pragma once

#include <optional>
#include <memory>
#include <string_view>

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
            Client(asio::io_context& ctx, ConnectionProcess& proc)
            {
                socket_type socket(ctx);
                connection = MakeConnection(std::move(socket), proc);
            }

            Client(asio::io_context& ctx, asio::ssl::context& ssl, ConnectionProcess& proc)
            {
                asio::ssl::stream<socket_type> socket(ctx, ssl);
                connection = MakeConnection(std::move(socket), proc);
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
            void Send(std::span<T> msg)
            {
                //TODO: outgoing messages should be const but compiler gives errors when copying the span to the outgoing queue...
                connection->Send(std::as_writable_bytes(msg));
            }

        private:                        
            std::shared_ptr<ConnectionInterface> connection;
        };
    }
}