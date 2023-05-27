/**
 * @file AwaitableServer.h
 * @author David Brill (6david6brill6@gmail.com)
 * 
 * @copyright Copyright (c) 2023
 * Distributed under the Apache License 2.0 (see accompanying
 * file LICENSE or copy at http://www.apache.org/licenses/)
 * 
 * @brief Defines the AwaitableServer class
 */

#pragma once

#include <charconv>

#include "AwaitableConnection.h"
#include "EndpointHelper.h"

namespace Brilliant
{
    namespace Network
    {
        /**
         * @class AwaitableServer
         * @brief Provides an interface for awaitable server types, accepts incoming connections
         * @tparam Protocol The protocol implementation type
         */
        template<class Protocol>
        class AwaitableServer
        {
        public:
            using protocol_type = Protocol;
            using base_protocol_type = typename Protocol::protocol_type;
            using acceptor_type = typename protocol_type::acceptor_type;
            using connection_type = AwaitableConnection<protocol_type>;

            /**
             * @brief Construct a new Awaitable Server object
             * 
             * @param e The executor for asio coroutines spawned by the AwaitableServer object
             */
            AwaitableServer(asio::any_io_executor e) : 
                executor(e)
            {

            }

            /**
             * @brief Destroy the Awaitable Server object
             * 
             */
            ~AwaitableServer()
            {
                Disconnect();
            }

            /**
             * @brief Stop all acceptors and disconnect all active connections
             * 
             */
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
            /**
             * @brief Accept connections on the given service
             * @param service The service to accept connections on as a string
             * @return A generator of pointers to connections
             */
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

            /**
             * @brief Accept connections on the given service
             * @param service The service to accept connections on as a string
             * @return A generator of pointers to connections
             */
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
            /**
             * @brief Accept ssl wrapped connections on the given service 
             * @param service The service to accept on as a string
             * @param ssl The ssl context to use for incoming connections
             * @return A generator of pointers to connections
             */
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

            /**
             * @brief Get the executor object. Allows declaration of member asio coroutines 
             * without needing the executor as the first parameter
             * 
             * @return The executor
             */
            auto get_executor()
            {
                return executor;
            }

        private:
            /**
             * @brief Initialize an acceptor
             * 
             * @param acceptor 
             * @param service 
             * @param ec 
             */
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

            //! The asio executor used for asio coroutines
            asio::any_io_executor executor;

            //! A vector of any acceptors created and managed by the AwaitableServer
            std::vector<acceptor_type> acceptors;

            //! A vector of connections created and managed by the AwaitableServer
            std::vector<connection_type> connections;
        };
    }
}