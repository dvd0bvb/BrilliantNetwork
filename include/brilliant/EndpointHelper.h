/**
 * @file EndpointHelper.h
 * @author David Brill (6david6brill6@gmail.com)
 * 
 * @copyright Copyright (c) 2023
 * Distributed under the Apache License 2.0 (see accompanying
 * file LICENSE or copy at http://www.apache.org/licenses/)
 * 
 * @brief Defines functions to help with internal endpoint creation
 */

#pragma once

#include <charconv>

#include "AsioIncludes.h"
#include "SocketTraits.h"

namespace Brilliant
{
    namespace Network
    {
        //TODO: this assumes the service is a number, what if I want to use "http", "amqp", etc?
        /**
         * @brief Create an endpoint from a service string
         * 
         * @tparam Protocol The asio protocol type
         * @param service The service as a string
         * @param ec[out] The error_code object an error will be stored in if there is one
         * @return An endpoint created from the service
         */
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

        /**
         * @brief Create an endpoint from a service string
         * 
         * @tparam Protocol The asio protocol type
         * @param service The service as a string
         * @param ec Unused
         * @return An endpoint created from the service
         */
        template<class Protocol>
        auto MakeEndpointFromService(std::string_view service, error_code&) requires (is_local_protocol_v<typename Protocol::protocol_type>)
        {
            return typename Protocol::endpoint_type{ service };
        }

        /**
         * @brief Resolve endpoints from host and service strings
         * 
         * @tparam Protocol The asio protocol type
         * @param exec The asio executor for the resolver
         * @param host The host as a string
         * @param service The service as a string
         * @param[out] ec An error_code that an error will be stored in if they occur
         */
        template<class Protocol>
        asio::awaitable<typename Protocol::resolver_type::results_type>
            ResolveEndpoints(asio::any_io_executor exec, std::string_view host, std::string_view service, error_code& ec)
            requires (!is_local_protocol_v<typename Protocol::protocol_type>)
        {
            typename Protocol::resolver_type resolver(exec);
            co_return co_await resolver.async_resolve(host, service, asio::redirect_error(asio::use_awaitable, ec));
        }

        /**
         * @brief Resolve endpoints from host and service strings
         * 
         * @tparam Protocol The asio protocol type
         * @param exec The asio executor for the resolver
         * @param host The host as a string
         * @param service The service as a string
         * @param[out] ec An error_code that errors will be stored in if they occur
         */
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