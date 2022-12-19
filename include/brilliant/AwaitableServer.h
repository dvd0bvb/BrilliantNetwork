#pragma once

#include <charconv>

#include "AwaitableConnection.h"
#include "EndpointHelper.h"

namespace Brilliant
{
    namespace Network
    {
        template<class Protocol>
        class AwaitableServer
        {
        public:
            using protocol_type = Protocol;
            using acceptor_type = asio::basic_socket_acceptor<protocol_type>;

            AwaitableServer(asio::io_context& ctx) : 
                context(ctx)
            {

            }

            //TODO: Consider returning a ref to the acceptor so we can stop it
            asio::experimental::generator<AwaitableConnectionInterface*>
                AcceptOn(std::string_view service) 
                requires (!is_datagram_protocol_v<protocol_type>)
            {
                asio::error_code ec{};
                auto& acceptor = acceptors.emplace_back(context); //use basic_socket_acceptor in case of generic protocol
                InitAcceptor(acceptor, service, ec);
                if (ec)
                {
                    acceptors.pop_back();
                }

                while (!ec && acceptor.is_open())
                {
                    typename protocol_type::socket socket{ context };
                    //need to use this overload of async_accept because socket does not have a default ctor
                    //coro requires that yield value has default ctor
                    co_await acceptor.async_accept(socket, asio::experimental::use_coro);
                    auto& result = connections.emplace_back(std::make_unique<AwaitableConnection<typename protocol_type::socket>>(std::move(socket)));
                    co_yield result.get();
                }
            }

            asio::awaitable<AwaitableConnectionInterface*>
                AcceptOn(std::string_view service)
                requires (is_datagram_protocol_v<protocol_type>)
            {
                asio::error_code ec{};
                auto ep = MakeEndpointFromService<protocol_type>(service, ec);
                if (ec)
                {
                    co_return nullptr;
                }

                typename protocol_type::socket socket{ context, ep };
                auto& result = connections.emplace_back(std::make_unique<AwaitableConnection<typename protocol_type::socket>>(std::move(socket)));
                co_return result.get();
            }

            asio::experimental::generator<AwaitableConnectionInterface*>
                AcceptOn(std::string_view service, asio::ssl::context& ssl)
            {
                static_assert(!is_datagram_protocol_v<protocol_type>, "Cannot use ssl with a datagram protocol");

                asio::error_code ec{};
                auto& acceptor = acceptors.emplace_back(context); //use basic_socket_acceptor in case of generic protocol
                InitAcceptor(acceptor, service, ec);
                if (ec)
                {
                    acceptors.pop_back();
                }

                while (!ec && acceptor.is_open())
                {
                    typename protocol_type::socket socket{ context };
                    //need to use this overload of async_accept because socket does not have a default ctor
                    //coro requires that yield value has default ctor
                    co_await acceptor.async_accept(socket, asio::experimental::use_coro);

                    asio::ssl::stream<typename protocol_type::socket> ssl_socket{ std::move(socket), ssl };
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
            std::vector<acceptor_type> acceptors;
            std::vector<std::unique_ptr<AwaitableConnectionInterface>> connections;
        };
    }
}