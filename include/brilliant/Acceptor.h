#pragma once

#include <memory>
#include <string_view>
#include <charconv>

#include "Connection.h"
#include "ServerProcess.h"
#include "Ssl.h"

namespace Brilliant
{
    namespace Network
    {
        class AcceptorInterface : public std::enable_shared_from_this<AcceptorInterface>
        {
        public:
            virtual void AcceptOn(std::string_view service) = 0;
            virtual void AcceptOn(std::string_view service, asio::ssl::context& ssl_context) = 0;
            virtual void Stop() = 0;
        };

        template<class Protocol> requires (!is_datagram_protocol_v<Protocol>)
        class Acceptor : public AcceptorInterface
        {
        private:
            struct allocate_shared_enabler : public Acceptor
            {
                template<class... Args> allocate_shared_enabler(Args&&... args) : Acceptor(std::forward<Args>(args)...) {}
            };

        public:
            using protocol_type = Protocol;
            using socket_type = typename protocol_type::socket;
            using acceptor_type = typename protocol_type::acceptor;

            template<class AcceptorAllocator = std::allocator<Acceptor>>
            static std::shared_ptr<Acceptor> Create(asio::io_context& ctx, ServerProcess& proc)
            {
                AcceptorAllocator allocator{};
                return std::allocate_shared<allocate_shared_enabler>(allocator, ctx, proc);
            }
            
            void AcceptOn(std::string_view service)
            {
                //use asio::detached because we don't need a handler when coroutine completes
                asio::co_spawn(context, std::bind(&Acceptor::DoAccept, shared_from_base(), service), asio::detached);
            }

            void AcceptOn(std::string_view service, asio::ssl::context& ssl_context)
            {
                //use asio::detached because we don't need a handler when coroutine completes
                asio::co_spawn(context, std::bind(&Acceptor::DoSslAccept, shared_from_base(), service, std::ref(ssl_context)), asio::detached);
            }

            void Stop()
            {
                asio::error_code ec{};
                acceptor.close(ec);
                if (ec)
                {
                    process.OnAcceptorError(*this, ec);
                }

                process.OnAcceptorClose(*this);
            }

        private:
            Acceptor(asio::io_context& ctx, ServerProcess& proc) : context(ctx), process(proc), acceptor(context)
            {

            }

            asio::awaitable<void> DoAccept(std::string_view service)
            {
                asio::error_code ec{};
                while (true)
                {
                    auto socket = co_await AsyncAccept(service, ec);
                    if (ec)
                    {
                        process.OnAcceptorError(*this, ec);
                        co_return;
                    }

                    auto connection = MakeConnection(std::move(socket), process);
                    process.OnAcceptedConnection(*connection);
                    connection->Connect();
                }
            }

            asio::awaitable<void> DoSslAccept(std::string_view service, asio::ssl::context& ssl_context)
            {
                asio::error_code ec{};
                while (true)
                {
                    auto socket = co_await AsyncAccept(service, ec);
                    if (ec)
                    {
                        process.OnAcceptorError(*this, ec);
                        co_return;
                    }

                    asio::ssl::stream<socket_type> ssl_socket{ std::move(socket), ssl_context };
                    auto connection = MakeConnection(std::move(ssl_socket), process);
                    process.OnAcceptedConnection(*connection);
                    connection->Connect();
                }
            }

            //TODO: break this up using concepts
            asio::awaitable<socket_type> AsyncAccept(std::string_view service, asio::error_code& ec)
            {
                //TODO: break endpoint init up into private functions. see AwaitableServier.h
#ifndef _WIN32
                if constexpr (std::is_same_v<asio::local::stream_protocol, protocol_type>)
                {
                    typename protocol_type::endpoint ep{ service };
                    acceptor.open(ep.protocol());
                    acceptor.bind(ep, ec);
                    if (ec)
                    {
                        co_return socket_type{ context };
                    }

                    acceptor.listen();

                    co_return acceptor.async_accept(asio::redirect_error(asio::use_awaitable, ec));
                }
                else
#endif
                {
                    std::uint16_t port{};
                    std::from_chars(service.data(), service.data() + service.size(), port);
                    //TODO: check result of from_chars
                    typename protocol_type::endpoint ep{ protocol_type::v4(), port };
                    acceptor.open(ep.protocol());
                    acceptor.bind(ep, ec);
                    if (ec)
                    {
                        co_return socket_type{ context };
                    }

                    acceptor.listen();

                    co_return co_await acceptor.async_accept(asio::redirect_error(asio::use_awaitable, ec));
                }
            }

            std::shared_ptr<Acceptor> shared_from_base()
            {
                return std::static_pointer_cast<Acceptor>(shared_from_this());
            }

            asio::io_context& context;
            ServerProcess& process;
            acceptor_type acceptor;
        };
    }
}