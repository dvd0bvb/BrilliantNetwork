#pragma once

#include <memory>
#include <vector>
#include <any>
#include <string_view>
#include <span>
#include <queue>

#include "AsioIncludes.h"
#include "ConnectionProcess.h"
#include "SocketTraits.h"

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
            virtual void Send(std::span<std::byte> span) = 0;
        };

        template<class Socket, class BufferAllocator = std::allocator<std::byte>>
        class BasicConnection : public ConnectionInterface
        {
        private:
            struct allocate_shared_enabler : public BasicConnection
            {
                template<class... Args> allocate_shared_enabler(Args&&... args) : BasicConnection(std::forward<Args>(args)...) {}
            };

        public:
            using socket_type = Socket;
            using protocol_type = socket_protocol_type_t<socket_type>;
            using buffer_allocator_type = BufferAllocator;

            template<class ConnectionAllocator = std::allocator<BasicConnection>>
            static std::shared_ptr<BasicConnection> Create(socket_type socket, ConnectionProcess& process)
            {
                ConnectionAllocator allocator{};
                return std::allocate_shared<allocate_shared_enabler>(allocator, std::move(socket), process);
            }

            void Connect()
            {
                //use asio::detached because we don't need a handler when coroutine completes
                asio::co_spawn(strand, std::bind(&BasicConnection::OnIncomingConnect, shared_from_base()), asio::detached);
            }

            void Connect(std::string_view host, std::string_view service)
            {
                //use asio::detached because we don't need a handler when coroutine completes
                asio::co_spawn(strand, std::bind(&BasicConnection::OnOutgoingConnect, shared_from_base(), host, service), asio::detached);
            }

            void Disconnect()
            {
                asio::error_code ec;
                socket.lowest_layer().cancel(ec);
                if (ec)
                {
                    OnError(ec);
                }

                if (IsConnected())
                {
                    if constexpr (is_ssl_wrapped_v<socket_type>)
                    {
                        socket.shutdown(ec);
                        if (ec)
                        {
                            OnError(ec);
                        }
                    }

                    socket.lowest_layer().shutdown(socket_type::lowest_layer_type::shutdown_both, ec);
                    if (ec)
                    {
                        OnError(ec);
                    }

                    socket.lowest_layer().close(ec);
                    if (ec)
                    {
                        OnError(ec);
                    }
                }
            }

            bool IsConnected() const
            {
                if constexpr (is_ssl_wrapped_v<socket_type>)
                {
                    return socket.lowest_layer().is_open();
                }
                else
                {
                    return socket.is_open();
                }
            }

            void Send(std::span<std::byte> msg)
            {
                const bool writing = !out_queue.empty();
                out_queue.emplace(msg.begin(), msg.end());
                if (connect_complete && !writing)
                {
                    //use asio::detached because we don't need a handler when coroutine completes
                    asio::co_spawn(strand, std::bind(&BasicConnection::Write, shared_from_base()), asio::detached);
                }
            }

        private:
            BasicConnection(socket_type sock, ConnectionProcess& proc) : connect_complete(false), socket(std::move(sock)), process(proc), strand(socket.get_executor())
            {

            }

            asio::awaitable<void> OnIncomingConnect()
            {
                if constexpr (is_ssl_wrapped_v<socket_type>)
                {
                    asio::error_code ec;
                    co_await socket.async_handshake(socket.server, asio::redirect_error(asio::use_awaitable, ec));
                    if (ec)
                    {
                        OnError(ec);
                        co_return;
                    }
                }

                //dg protocols need to wait for read in order to send. See Read()
                if constexpr (!is_datagram_protocol_v<protocol_type>)
                { 
                    OnConnectComplete();
                }

                asio::co_spawn(strand, std::bind(&BasicConnection::Read, shared_from_base()), asio::detached);
                co_return;
            }

            asio::awaitable<void> OnOutgoingConnect(std::string_view host, std::string_view service)
            {
                asio::error_code ec;
                asio::ip::basic_resolver<protocol_type> resolver(strand);
                typename asio::ip::basic_resolver<protocol_type>::results_type results{};

#ifndef _WIN32
                //if using local protocol on posix system, need to resolve only the path
                if constexpr (std::is_same_v<protocol_type, asio::local::stream_protocol> || std::is_same_v<protocol_type, asio::local::datagram_protocol>)
                {
                    results = co_await resolver.async_resolve({ service }, asio::redierect_error(asio::use_awaitable, ec);
                }
                else
#endif
                {
                    results = co_await resolver.async_resolve(host, service, asio::redirect_error(asio::use_awaitable, ec));
                }

                if (ec)
                {
                    OnError(ec);
                    co_return;
                }

                remote_endpoint = co_await asio::async_connect(socket.lowest_layer(), results, asio::redirect_error(asio::use_awaitable, ec));
                if (ec)
                {
                    OnError(ec);
                    co_return;
                }

                if constexpr (is_ssl_wrapped_v<socket_type>)
                {
                    co_await socket.async_handshake(socket.client, asio::redirect_error(asio::use_awaitable, ec));
                    if (ec)
                    {
                        OnError(ec);
                        co_return;
                    }
                }

                OnConnectComplete();

                asio::co_spawn(strand, std::bind(&BasicConnection::Read, shared_from_base()), asio::detached);
            }

            asio::awaitable<void> Read()
            {
                while (true)
                {
                    asio::error_code ec;
                    std::size_t bytes_read = 0;
                    std::vector<std::byte, buffer_allocator_type> in_buffer;

                    //dg sockets that don't have connect() called on them (such as in the case when Connect() with no params is called) will need to wait until they receive
                    //at which point remote_endpoint will be set and data will be able to be sent back
                    if constexpr (is_datagram_protocol_v<protocol_type>)
                    {
                        std::size_t bytes_left = ReadCompletion(ec, bytes_read);

                        do {
                            in_buffer.insert(in_buffer.cend(), bytes_left, std::byte{});
                            bytes_read += co_await socket.async_receive_from(asio::buffer(in_buffer), remote_endpoint, asio::redirect_error(asio::use_awaitable, ec));
                            bytes_left = ReadCompletion(ec, bytes_read);
                        } while (bytes_left > 0 && !ec && bytes_read != 0);

                        if (!connect_complete)
                        {
                            OnConnectComplete();
                        }
                    }
                    else
                    {
                        bytes_read = co_await asio::async_read(socket, asio::dynamic_vector_buffer(in_buffer), std::bind(&BasicConnection::ReadCompletion, this, std::placeholders::_1, std::placeholders::_2), asio::redirect_error(asio::use_awaitable, ec));
                    }

                    if (ec)
                    {
                        OnError(ec);
                        co_return;
                    }

                    process.OnRead(*this, in_buffer);
                }
            }

            std::size_t ReadCompletion(const asio::error_code& ec, std::size_t n)
            {
                return process.ReadCompletion(*this, ec, n);
            }

            asio::awaitable<void> Write()
            {
                while (!out_queue.empty())
                {
                    asio::error_code ec;
                    std::size_t bytes_written = 0;
                    const auto& data = out_queue.front();

                    if constexpr (is_datagram_protocol_v<protocol_type>)
                    {
                        bytes_written = co_await socket.async_send_to(asio::buffer(data), remote_endpoint, asio::redirect_error(asio::use_awaitable, ec));
                    }
                    else
                    {
                        bytes_written = co_await asio::async_write(socket, asio::buffer(data), asio::redirect_error(asio::use_awaitable, ec));
                    }

                    if (ec)
                    {
                        OnError(ec);
                        co_return;
                    }
                    
                    process.OnWrite(*this, bytes_written);
                    out_queue.pop();
                }
            }

            void OnError(asio::error_code& ec, const std::source_location& loc = std::source_location::current())
            {
                process.OnError(*this, ec, loc);
                Disconnect();
            }

            void OnConnectComplete()
            {
                connect_complete = true;

                process.OnConnect(*this);

                if (!out_queue.empty())
                {
                    asio::co_spawn(strand, std::bind(&BasicConnection::Write, shared_from_base()), asio::detached);
                }
            }

            std::shared_ptr<BasicConnection> shared_from_base()
            {
                return std::static_pointer_cast<BasicConnection>(this->shared_from_this());
            }

            bool connect_complete;
            ConnectionProcess& process;
            typename protocol_type::endpoint remote_endpoint;
            socket_type socket;
            asio::strand<typename socket_type::executor_type> strand;
            std::queue<std::vector<std::byte, buffer_allocator_type>> out_queue;
        };

        template<class Socket, class BufferAllocator = std::allocator<std::byte>, class ConnectionAllocator = std::allocator<BasicConnection<Socket, BufferAllocator>>>
        std::shared_ptr<BasicConnection<Socket, BufferAllocator>> MakeConnection(Socket socket, ConnectionProcess& process)
        {
            return BasicConnection<Socket, BufferAllocator>::Create(std::move(socket), process);
        }
    }
}