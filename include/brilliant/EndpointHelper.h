#pragma once

#include "AsioIncludes.h"
#include "SocketTraits.h"

namespace Brilliant
{
    namespace Network
    {
        template<class Protocol>
        auto MakeEndpointFromService(std::string_view service, error_code& ec) requires (!is_local_protocol_v<Protocol>)
        {
            std::uint16_t port{};
            auto [ptr, err] = std::from_chars(service.data(), service.data() + service.size(), port);
            if (err != std::errc{})
            {
                ec = std::make_error_code(err);
            }

            return typename Protocol::endpoint{ Protocol::v4(), port };
        }

        template<class Protocol>
        auto MakeEndpointFromService(std::string_view service, error_code&) requires (is_local_protocol_v<Protocol>)
        {
            return typename Protocol::endpoint{ service };
        }

        template<class Protocol>
        asio::awaitable<typename asio::ip::basic_resolver<Protocol>::results_type>
            ResolveEndpoints(asio::any_io_executor exec, std::string_view host, std::string_view service, error_code& ec)
            requires (!is_local_protocol_v<Protocol>)
        {
            asio::ip::basic_resolver<Protocol> resolver(exec);
            co_return co_await resolver.async_resolve(host, service, asio::redirect_error(asio::use_awaitable, ec));
        }

        template<class Protocol>
        asio::awaitable<typename asio::ip::basic_resolver<Protocol>::results_type>
            ResolveEndpoints(asio::any_io_executor exec, std::string_view host, std::string_view service, error_code& ec)
            requires(is_local_protocol_v<Protocol>)
        {
            asio::ip::basic_resolver<Protocol> resolver(exec);
            co_return co_await resolver.async_resolve({ service }, asio::redirect_error(asio::use_awaitable, ec));
        }
    }
}