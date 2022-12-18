#pragma once

#include <memory>
#include <string_view>
#include <span>

#include "AsioIncludes.h"

namespace Brilliant
{
    namespace Network
    {
        class AwaitableConnectionInterface
        {
        public:
            virtual ~AwaitableConnectionInterface() = default;
            virtual asio::awaitable<asio::error_code> Connect() = 0;
            virtual asio::awaitable<asio::error_code> Connect(std::string_view host, std::string_view service) = 0;
            virtual asio::error_code Disconnect() = 0; //disconnect will always be synchronous
            virtual bool IsConnected() const = 0;
            //TODO: can we make this a range instead of a span?
            virtual asio::awaitable<std::pair<std::size_t, asio::error_code>> Send(std::span<const std::byte> data) = 0;
            virtual asio::awaitable<std::pair<std::size_t, asio::error_code>> ReadInto(std::span<std::byte> data) = 0;
        };
    }
}