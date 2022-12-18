#pragma once

#include <string_view>
#include <charconv>

#include "ServerProcess.h"
#include "Acceptor.h"

namespace Brilliant
{
    namespace Network
    {
        class Server
        {
        public:
            Server(asio::io_context& ctx, ServerProcess& proc) : 
                context(ctx), 
                process(proc)
            {

            }

            //TODO: Look into adding options such as multicast, ssl
            template<class Protocol>
            void AcceptOn(std::string_view service)
            {
                if constexpr (is_datagram_protocol_v<Protocol>)
                {
#ifndef _WIN32
                    if constexpr (std::is_same_v<asio::local::datagram_protocol, protocol_type>)
                    {
                        typename Protocol::socket socket(context, { service });
                        auto connection = MakeConnection(std::move(socket), process);
                        process.OnBeginListen(*connection);
                        connection->Connect();
                    }
                    else
#endif
                    {
                        uint16_t port{};
                        std::from_chars(service.data(), service.data() + service.size(), port);
                        typename Protocol::socket socket(context, { Protocol::v4(), port });
                        auto connection = MakeConnection(std::move(socket), process);
                        process.OnBeginListen(*connection);
                        connection->Connect();
                    }
                }
                else
                {
                    auto acceptor = Acceptor<Protocol>::Create(context, process);
                    acceptor->AcceptOn(service);
                }
            }

            template<class Protocol>
            void AcceptOn(std::string_view service, asio::ssl::context& ssl_context)
            {
                static_assert(!is_datagram_protocol_v<Protocol>, "Cannot use ssl with a datagram protocol");
                auto acceptor = Acceptor<Protocol>::Create(context, process);
                acceptor->AcceptOn(service, ssl_context);
            }

        private:
            asio::io_context& context;
            ServerProcess& process;
            std::vector<std::weak_ptr<ConnectionInterface>> connections;
            std::vector<std::weak_ptr<AcceptorInterface>> acceptors;
        };
    }
}