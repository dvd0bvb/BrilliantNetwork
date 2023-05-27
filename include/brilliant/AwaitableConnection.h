/**
 * @file AwaitableConnection.h
 * @author David Brill (6david6brill6@gmail.com)
 * 
 * @copyright Copyright (c) 2023
 * Distributed under the Apache License 2.0 (see accompanying
 * file LICENSE or copy at http://www.apache.org/licenses/)
 * 
 * @brief Provides the AwaitableConnection class template
 */

#pragma once

#include <concepts>

#include "AsioIncludes.h"
#include "SocketTraits.h"
#include "EndpointHelper.h"

namespace Brilliant
{
    namespace Network
    {
        /**
         * @class AwaitableConnection
         * @brief Provides an interface for connections on the given protocol
         * @tparam Protocol The protocol implementation type
         */
        template<class Protocol>
        class AwaitableConnection
        {        
        public:
            using protocol_type = Protocol;
            using socket_type = typename protocol_type::socket_type;

            /**
             * @brief Construct a new Awaitable Connection object
             * 
             * @param sock The socket to connect on
             */
            AwaitableConnection(socket_type sock) : 
                socket(std::move(sock))
            {

            }

            /**
             * @brief Connect using the socket
             * 
             * @return The first error that occurred during connection if there was one
             */
            asio::awaitable<error_code> Connect()
            {
                return impl.Connect(socket);
            }

            /**
             * @brief Connect to the endpoint via the host and service
             * 
             * @param host The host to connect to 
             * @param service The service to connect to
             * @return The first error that occurred during connection if there was one 
             */
            asio::awaitable<error_code> Connect(std::string_view host, std::string_view service)
            {
                auto result = co_await impl.Connect(socket, host, service);
                remote_endpoint = std::get<typename protocol_type::endpoint_type>(result);
                co_return std::get<error_code>(result);
            }

            /**
             * @brief Disconnect and close the underlying socket
             * 
             * @return The first error that occurred during disconnection if there was one
             */
            error_code Disconnect()
            {
                return impl.Disconnect(socket);
            }

            /**
             * @brief Tells if the connection is active
             * 
             * @return True if the connection is active, false otherwise 
             */
            bool IsConnected() const
            {
                return impl.IsConnected(socket);
            }

            /**
             * @brief Send data on the socket
             * 
             * @tparam T The message type
             * @param data The message
             * @return The number of bytes written to the socket and the first error to occur if there was one
             */
            template<class T>
            asio::awaitable<std::pair<std::size_t, error_code>> Send(T&& data)
            {
                if constexpr (is_datagram_protocol_v<typename protocol_type::protocol_type>)
                {
                    return impl.Send(socket, remote_endpoint, std::forward<T>(data));
                }
                else
                {
                    return impl.Send(socket, std::forward<T>(data));
                }
            }

            /**
             * @brief Read data from the socket into a message
             * 
             * @tparam T The message type
             * @param data The message
             * @return The number of bytes read and the first error to occur if there was one
             */
            template<class T>
            asio::awaitable<std::pair<std::size_t, error_code>> ReadInto(T&& data)
            {
                if constexpr (is_datagram_protocol_v<typename protocol_type::protocol_type>)
                {
                    return impl.ReadInto(socket, remote_endpoint, std::forward<T>(data));
                }
                else
                {
                    return impl.ReadInto(socket, std::forward<T>(data));
                }
            }

        private:
            //! The underlying socket
            socket_type socket;

            //! The remote endpoint 
            typename protocol_type::endpoint_type remote_endpoint;

            //! The protocol implementation 
            protocol_type impl;
        };
    }
}