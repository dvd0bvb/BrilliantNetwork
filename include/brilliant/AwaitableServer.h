#pragma once

#include <charconv>

#include "AwaitableConnection.h"

namespace Brilliant
{
    namespace Network
    {
        class AwaitableServer
        {
        public:
            AwaitableServer(asio::io_context& ctx) : 
                context(ctx)
            {

            }

            //TODO: Consider returning a ref to the acceptor so we can stop it
            template<class Protocol>
            asio::experimental::generator<AwaitableConnectionInterface*>
                AcceptOn(std::string_view service) 
                requires (!is_datagram_protocol_v<Protocol>)
            {
                asio::basic_socket_acceptor<Protocol> acceptor(context); //use basic_socket_acceptor in case of generic protocol
                auto ep = MakeEndpoint<Protocol>(service);
                acceptor.open(ep.protocol());
                acceptor.bind(ep); //this can throw, should use error code overload
                acceptor.listen();

                while (acceptor.is_open())
                {
                    typename Protocol::socket socket{ context };
                    //need to use this overload of async_accept because socket does not have a default ctor
                    //coro requires that yield value has default ctor
                    co_await acceptor.async_accept(socket, asio::experimental::use_coro);
                    auto& result = connections.emplace_back(std::make_unique<AwaitableConnection<typename Protocol::socket>>(std::move(socket)));
                    co_yield result.get();
                }
            }

            template<class Protocol>
            asio::awaitable<AwaitableConnectionInterface*>
                AcceptOn(std::string_view service)
                requires (is_datagram_protocol_v<Protocol>)
            {
                auto ep = MakeEndpoint<Protocol>(service);
                typename Protocol::socket socket{ context, ep };
                auto& result = connections.emplace_back(std::make_unique<AwaitableConnection<typename Protocol::socket>>(std::move(socket)));
                co_return result.get();
            }

            template<class Protocol>
            asio::experimental::generator<AwaitableConnectionInterface*>
                AcceptOn(std::string_view service, asio::ssl::context& ssl)
            {
                static_assert(!is_datagram_protocol_v<Protocol>, "Cannot use ssl with a datagram protocol");

                asio::basic_socket_acceptor<Protocol> acceptor(context); //use basic_socket_acceptor in case of generic protocol
                auto ep = MakeEndpoint<Protocol>(service);
                acceptor.open(ep.protocol());
                acceptor.bind(ep); //this can throw, should use error code overload
                acceptor.listen();

                while (acceptor.is_open())
                {
                    typename Protocol::socket socket{ context };
                    //need to use this overload of async_accept because socket does not have a default ctor
                    //coro requires that yield value has default ctor
                    co_await acceptor.async_accept(socket, asio::experimental::use_coro);

                    asio::ssl::stream<typename Protocol::socket> ssl_socket{ std::move(socket), ssl };
                    auto& result = connections.emplace_back(std::make_unique<AwaitableConnection<std::decay_t<decltype(ssl_socket)>>>(std::move(ssl_socket)));
                    co_yield result.get();
                }
            }

            //allows declaration of member coros without needing executor as first argument
            auto get_executor()
            {
                return context.get_executor();
            }

        private:
            template<class Protocol>
            requires (!is_local_protocol_v<Protocol>)
            auto MakeEndpoint(std::string_view service)
            {
                std::uint16_t port{};
                auto result = std::from_chars(service.data(), service.data() + service.size(), port);
                //TODO: check result
                //TODO: see if we can use other types, like v6(), perhaps function parameter
                return typename Protocol::endpoint{ Protocol::v4(), port };
            }

            template<class Protocol>
            requires (is_local_protocol_v<Protocol>)
            auto MakeEndpoint(std::string_view service)
            {
                return typename Protocol::endpoint{ service };
            }

            asio::io_context& context;
            std::vector<std::unique_ptr<AwaitableConnectionInterface>> connections;
        };
    }
}