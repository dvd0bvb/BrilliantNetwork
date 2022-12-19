#pragma once

#include <string_view>

#include "AsioIncludes.h"

namespace Brilliant
{
    namespace Network
    {
        class AwaitableConnectionInterface
        {
        public:
            virtual ~AwaitableConnectionInterface() = default;
            virtual asio::awaitable<error_code> Connect() = 0;
            virtual asio::awaitable<error_code> Connect(std::string_view host, std::string_view service) = 0;
            virtual error_code Disconnect() = 0; //disconnect will always be synchronous
            virtual bool IsConnected() const = 0;
            virtual asio::awaitable<std::pair<std::size_t, error_code>> Send(const asio::const_buffer& data) = 0;
            virtual asio::awaitable<std::pair<std::size_t, error_code>> ReadInto(const asio::mutable_buffer& data) = 0;
        };
    }
}