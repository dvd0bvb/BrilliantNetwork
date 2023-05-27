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
            using base_protocol_type = typename Protocol::protocol_type;
            using acceptor_type = typename protocol_type::acceptor_type;
            using connection_type = AwaitableConnection<protocol_type>;

            AwaitableServer(asio::any_io_executor e) : 
                executor(e)
            {

            }

            ~AwaitableServer()
            {
                Disconnect();
            }

            void Disconnect()
            {
                for (auto& acceptor : acceptors)
                {
                    acceptor.cancel();
                }

                for (auto& connection : connections)
                {
                    connection.Disconnect();
                }
            }

            //TODO: Consider returning a ref/handle to the acceptor so we can stop it
            asio::experimental::generator<connection_type*>
                AcceptOn(std::string_view service) 
                requires (!is_datagram_protocol_v<base_protocol_type>)
            {
                static_assert(!is_ssl_wrapped_v<typename protocol_type::socket_type>, "Cannot use protocol with socket type of asio::ssl::stream<T> with this overload");

                error_code ec{};
                auto& acceptor = acceptors.emplace_back(co_await asio::this_coro::executor); //use basic_socket_acceptor in case of generic protocol
                InitAcceptor(acceptor, service, ec);
                if (ec)
                {
                    acceptors.pop_back();
                }

                while (!ec && acceptor.is_open())
                {
                    typename protocol_type::socket_type socket{ co_await asio::this_coro::executor };
                    co_await protocol_type::Accept(acceptor, socket);
                    auto& result = connections.emplace_back(std::move(socket));
                    co_yield &result;
                }
            }

            asio::awaitable<connection_type*>
                AcceptOn(std::string_view service)
                requires (is_datagram_protocol_v<base_protocol_type>)
            {
                error_code ec{};
                auto ep = MakeEndpointFromService<protocol_type>(service, ec);
                if (ec)
                {
                    co_return nullptr;
                }

                typename protocol_type::socket_type socket{ co_await asio::this_coro::executor, ep };
                auto& result = connections.emplace_back(std::move(socket));
                co_return &result;
            }

            //TODO: return type should include error code
            //in case of error should return nullptr and the error
            asio::experimental::generator<connection_type*>
                AcceptOn(std::string_view service, asio::ssl::context& ssl)
            {
                static_assert(!is_datagram_protocol_v<base_protocol_type>, "Cannot use ssl with a datagram protocol");
                static_assert(is_ssl_wrapped_v<typename protocol_type::socket_type>, "Must provide Protocol socket type of asio::ssl::stream<T>");

                error_code ec{};
                auto& acceptor = acceptors.emplace_back(co_await asio::this_coro::executor); //use basic_socket_acceptor in case of generic protocol
                InitAcceptor(acceptor, service, ec);
                if (ec)
                {
                    acceptors.pop_back();
                }

                while (acceptor.is_open())
                {
                    typename base_protocol_type::socket socket{ co_await asio::this_coro::executor };
                    // //need to use this overload of async_accept because socket does not have a default ctor
                    // //coro requires that yield value has default ctor
                    co_await acceptor.async_accept(socket, asio::experimental::use_coro); //TODO: this will throw, asio::redirect_error() will not compile

                    typename protocol_type::socket_type ssl_socket{ std::move(socket), ssl };
                    auto& result = connections.emplace_back(std::move(ssl_socket));
                    co_yield &result;
                }
            }

            //allows declaration of member coros without needing executor as first argument
            auto get_executor()
            {
                return executor;
            }

        private:
            void InitAcceptor(acceptor_type& acceptor, std::string_view service, error_code& ec)
            {
                //TODO: use resolver here?
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

            asio::any_io_executor executor;
            std::vector<acceptor_type> acceptors;
            std::vector<connection_type> connections;
        };
    }
}