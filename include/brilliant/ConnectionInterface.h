#pragma once

#include <memory>
#include <string_view>
#include <span>

namespace Brilliant
{
    namespace Network
    {
        class ConnectionInterface : public std::enable_shared_from_this<ConnectionInterface>
        {
        public:
            virtual ~ConnectionInterface() = default;
            virtual void Connect() = 0;
            virtual void Connect(std::string_view host, std::string_view service) = 0;
            virtual void Disconnect() = 0;
            virtual bool IsConnected() const = 0;
            virtual void Send(std::span<std::byte> data) = 0;
        };
    }
}