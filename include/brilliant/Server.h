#pragma once

#include <string_view>
#include <charconv>

#include "ServerProcess.h"
#include "EndpointHelper.h"

namespace Brilliant
{
    namespace Network
    {
        template<class Protocol>
        class Server
        {
        public:
            using protocol_type = Protocol;
            using acceptor_type = asio::basic_socket_acceptor<protocol_type>;

            Server(asio::io_context& ctx, ServerProcess& proc) : 
                context(ctx), 
                process(proc)
            {

            }

            ~Server()
            {
                for (auto& acceptor : acceptors)
                {
                    acceptor.close();              
                }

                for (auto& conn : connections)
                {
                    conn->Disconnect();
                }
            }

            //TODO: Look into adding options such as multicast, ssl
            void AcceptOn(std::string_view service) requires (!is_datagram_protocol_v<protocol_type>)
            {
                asio::co_spawn(context, [this, service]() mutable -> asio::awaitable<void> {                    
                    asio::error_code ec{};
                    auto& acceptor = acceptors.emplace_back(context);
                    InitAcceptor(acceptor, service, ec);
                    if (ec)
                    {
                        //process.OnAcceptorError()
                        acceptors.pop_back();
                        co_return;
                    }

                    while (acceptor.is_open())
                    {
                        auto socket = co_await acceptor.async_accept(asio::redirect_error(asio::use_awaitable, ec));
                        if (ec)
                        {
                            //process.OnAcceptorError();
                            co_return;
                        }

                        auto& connection = connections.emplace_back(Connection<typename protocol_type::socket>::Create(std::move(socket), process));
                        process.OnAcceptedConnection(*connection);
                        connection->Connect();
                    }
                    }, asio::detached);
            }

            void AcceptOn(std::string_view service) requires (is_datagram_protocol_v<protocol_type>)
            {
                asio::error_code ec{};
                auto ep = MakeEndpointFromService<protocol_type>(service, ec);
                if (ec)
                {
                    return;
                }

                typename protocol_type::socket socket(context, ep);
                auto& connection = connections.emplace_back(Connection<typename protocol_type::socket>::Create(std::move(socket), process));
                process.OnBeginListen(*connection);
                connection->Connect();
            }

            void AcceptOn(std::string_view service, asio::ssl::context& ssl_context)
            {
                asio::co_spawn(context, [this, service, &ssl_context]() mutable -> asio::awaitable<void> {
                    asio::error_code ec{};
                    auto& acceptor = acceptors.emplace_back(context);
                    InitAcceptor(acceptor, service, ec);
                    if (ec)
                    {
                        //process.OnAcceptorError()
                        acceptors.pop_back();
                        co_return;
                    }

                    while (acceptor.is_open())
                    {
                        auto socket = co_await acceptor.async_accept(asio::redirect_error(asio::use_awaitable, ec));
                        if (ec)
                        {
                            //process.OnAcceptorError();
                            co_return;
                        }

                        asio::ssl::stream<typename protocol_type::socket> ssl_socket{ std::move(socket), ssl_context };
                        auto& connection = connections.emplace_back(Connection<std::decay_t<decltype(ssl_socket)>>::Create(std::move(ssl_socket), process));
                        process.OnAcceptedConnection(*connection);
                        connection->Connect();
                    }
                    }, asio::detached);
            }

        private:
            void InitAcceptor(acceptor_type& acceptor, std::string_view service, asio::error_code& ec)
            {
                auto ep = MakeEndpointFromService<protocol_type>(service, ec);
                if (ec)
                {
                    return;
                }

                acceptor.open(ep.protocol());
                acceptor.bind(ep, ec);
                if (ec)
                {
                    return;
                }

                acceptor.listen();
            }

            asio::io_context& context;
            ServerProcess& process;
            std::vector<std::shared_ptr<ConnectionInterface>> connections;
            std::vector<acceptor_type> acceptors;
        };
    }
}