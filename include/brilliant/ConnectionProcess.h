#pragma once

#include <memory>
#include <cstddef>
#include <source_location>
#include <span>

namespace Brilliant
{
    namespace Network
    {
        class ConnectionInterface;

        class ConnectionProcess
        {
        public:
            virtual ~ConnectionProcess() = default;

            //gives error and where the error occurred
            virtual void OnError(ConnectionInterface& connection, const asio::error_code& ec, const std::source_location& location) = 0;

            //return 0 to indicate when data transmission is complete
            //TODO: how do we actually know without the data?
            virtual std::size_t ReadCompletion(ConnectionInterface& connection, const asio::error_code& ec, std::size_t n) = 0;

            //called when data read operation is complete, handle the incoming data
            virtual void OnRead(ConnectionInterface& connection, const std::span<std::byte>& data) = 0;

            //called after write is complete, gives number of bytes written
            virtual void OnWrite(ConnectionInterface& connection, std::size_t n) = 0;

            virtual void OnConnect(ConnectionInterface& connection) = 0;

            virtual void OnTimeout(ConnectionInterface& connection) = 0;

            virtual void OnDisconnect(ConnectionInterface& connection) = 0;

            virtual void OnClose(ConnectionInterface& connection) = 0;
        };
    }
}