#pragma once

#include "AsioIncludes.h"
#include "SocketTraits.h"

namespace Brilliant
{
    namespace Network
    {
        //TODO: this assumes the service is a number, what if I want to use "http", "amqp", etc?
        template<class Protocol>
        auto MakeEndpointFromService(std::string_view service, error_code& ec) requires (!is_local_protocol_v<typename Protocol::protocol_type>)
        {
            std::uint16_t port{};
            auto [ptr, err] = std::from_chars(service.data(), service.data() + service.size(), port);
            if (err != std::errc{})
            {
                ec = std::make_error_code(err);
            }

            return typename Protocol::endpoint_type{ Protocol::protocol_type::v4(), port };
        }

        template<class Protocol>
        auto MakeEndpointFromService(std::string_view service, error_code&) requires (is_local_protocol_v<typename Protocol::protocol_type>)
        {
            return typename Protocol::endpoint_type{ service };
        }

        template<class Protocol>
        asio::awaitable<typename Protocol::resolver_type::results_type>
            ResolveEndpoints(asio::any_io_executor exec, std::string_view host, std::string_view service, error_code& ec)
            requires (!is_local_protocol_v<typename Protocol::protocol_type>)
        {
            typename Protocol::resolver_type resolver(exec);
            co_return co_await resolver.async_resolve(host, service, asio::redirect_error(asio::use_awaitable, ec));
        }

        template<class Protocol>
        asio::awaitable<typename Protocol::resolver_type::results_type>
            ResolveEndpoints(asio::any_io_executor exec, std::string_view host, std::string_view service, error_code& ec)
            requires(is_local_protocol_v<typename Protocol::protocol_type>)
        {
            typename Protocol::resolver_type resolver(exec);
            co_return co_await resolver.async_resolve({ service }, asio::redirect_error(asio::use_awaitable, ec));
        }
    }
}