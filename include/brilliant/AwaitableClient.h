/**
 * @file AwaitableClient.h
 * @author David Brill (6david6brill6@gmail.com)
 * 
 * @copyright Copyright (c) 2023
 * Distributed under the Apache License 2.0 (see accompanying
 * file LICENSE or copy at http://www.apache.org/licenses/)
 * 
 * @brief Defines the AwaitableClient class template
 */

#pragma once

#include <ranges>

#include "AwaitableConnection.h"

namespace Brilliant
{
    namespace Network
    {
        /**
         * @class AwaitableClient 
         * @brief Creates and manages a connection object for clients and
         * provides an interface for awaitable client types
         * @tparam Protocol The protocol implementation type
         */
        template<class Protocol>
        class AwaitableClient
        {
        public:
            using protocol_type = Protocol;
            using socket_type = typename protocol_type::socket_type;
            using connection_type = AwaitableConnection<protocol_type>;

            /**
             * @brief Construct a new Awaitable Client object
             * 
             * @param executor The asio executor for this object
             */
            AwaitableClient(asio::any_io_executor executor) : 
                connection(connection_type(socket_type{ executor }))
            {

            }

            /**
             * @brief Construct a new Awaitable Client object
             * 
             * @param executor The asio executor for this object
             * @param ssl The asio ssl context for this client
             */
            AwaitableClient(asio::any_io_executor executor, asio::ssl::context& ssl) : 
                connection(connection_type(socket_type{ executor, ssl }))
            {

            }

            /**
             * @brief Destroy the Awaitable Client object
             * 
             */
            ~AwaitableClient()
            {
                Disconnect();
            }

            /**
             * @brief Connect to an endpoint created using the given host and service
             * 
             * @param host The host as a string
             * @param service The service as a string
             * @return An error_code containing a connection error if there was one
             */
            auto Connect(std::string_view host, std::string_view service)
            {
                return connection.Connect(host, service);
            }

            /**
             * @brief Disconnect the client
             * 
             * @return An error_code containing the first error which occurred during disconnect if there was one 
             */
            error_code Disconnect()
            {
                return connection.Disconnect();
            }

            /**
             * @brief Send data via the connection
             * 
             * @tparam T The message type
             * @param msg The message
             * @return The number of bytes sent and an error_code containing any errors that occurred during sending 
             */
            template<class T>
            auto Send(T&& msg)
            {
                return connection.Send(std::forward<T>(msg));
            }

            /**
             * @brief Read data into a message
             * 
             * @tparam T The message type
             * @param msg The message
             * @return The number of bytes sent and an error_code containing any errors that occurred during reading 
             */
            template<class T>
            auto Read(T&& msg)
            {
                return connection.ReadInto(std::forward<T>(msg));
            }

        private:
            //!The connection
            connection_type connection;
        };
    }
}