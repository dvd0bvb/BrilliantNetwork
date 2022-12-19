#pragma once

#include <vector>
#include <deque>

#include <iostream>

#include "AsioIncludes.h"
#include "ConnectionInterface.h"
#include "ConnectionProcess.h"
#include "SocketTraits.h"
#include "EndpointHelper.h"
#include "AwaitableConnection.h"

namespace Brilliant
{
    namespace Network
    {
        template<class Socket>
        class Connection : public ConnectionInterface
        {
        private:
            struct allocate_shared_enabler : public Connection
            {
                template<class... Args> allocate_shared_enabler(Args&&... args) : Connection(std::forward<Args>(args)...) {}
            };

        public:
            using socket_type = Socket;
            using protocol_type = socket_protocol_type_t<socket_type>;
            using buffer_type = asio::const_buffer;
            using queue_type = std::deque<buffer_type>;

            template<class ConnectionAllocator = std::allocator<Connection>>
            static auto Create(socket_type socket, ConnectionProcess& proc, const ConnectionAllocator& alloc = {})
            {
                return std::allocate_shared<allocate_shared_enabler>(alloc, std::move(socket), proc);
            }

            void Connect()
            {
                //use asio::detached because we don't need a handler when coroutine completes
                asio::co_spawn(strand, std::bind(&Connection::OnIncomingConnect, shared_from_base()), asio::detached);
            }

            void Connect(std::string_view host, std::string_view service)
            {
                //use asio::detached because we don't need a handler when coroutine completes
                asio::co_spawn(strand, std::bind(&Connection::OnOutgoingConnect, shared_from_base(), host, service), asio::detached);
            }

            void Disconnect()
            {
                if (auto ec = connection.Disconnect())
                {
                    process.OnError(*this, ec, std::source_location::current());
                }
            }

            bool IsConnected() const
            {
                return connection.IsConnected();
            }

            void Send(const asio::const_buffer& msg)
            {
                const bool writing = !out_queue.empty();
                out_queue.emplace_back(msg);
                if (connect_complete && !writing)
                {
                    //use asio::detached because we don't need a handler when coroutine completes
                    asio::co_spawn(strand, std::bind(&Connection::Write, shared_from_base()), asio::detached);
                }
            }

        private:
            Connection(socket_type sock, ConnectionProcess& proc) :
                connect_complete(false),
                strand(sock.get_executor()),
                connection(std::move(sock)),
                process(proc)                
            {

            }

            asio::awaitable<void> OnIncomingConnect()
            {
                if (auto ec = co_await connection.Connect())
                {
                    OnError(ec);
                    co_return;
                }

                //dg protocols need to wait for read in order to send. See Read()
                if constexpr (!is_datagram_protocol_v<protocol_type>)
                { 
                    OnConnectComplete();
                }

                asio::co_spawn(strand, std::bind(&Connection::Read, shared_from_base()), asio::detached);
                co_return;
            }

            asio::awaitable<void> OnOutgoingConnect(std::string_view host, std::string_view service)
            {
                if (auto ec = co_await connection.Connect(host, service))
                {
                    OnError(ec);
                    co_return;
                }

                OnConnectComplete();

                asio::co_spawn(strand, std::bind(&Connection::Read, shared_from_base()), asio::detached);
            }

            asio::awaitable<void> Read()
            {
                while (true)
                {
                    std::vector<std::byte> in_buffer{ ReadCompletion({}, 0) }; //need a better way to do this
                    auto [bytes_read, ec] = co_await connection.ReadInto(asio::buffer(in_buffer));

                    if (ec)
                    {
                        OnError(ec);
                        co_return;
                    }

                    //dg sockets that don't have connect() called on them (such as in the case when Connect() with no params is called) will need to wait until they receive
                    //at which point remote_endpoint will be set and data will be able to be sent back
                    if (!connect_complete)
                    {
                        OnConnectComplete();
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
                    const auto& data = out_queue.front();
                    auto [bytes_written, ec] = co_await connection.Send(data);
                    if (ec)
                    {
                        OnError(ec);
                        co_return;
                    }
                    
                    process.OnWrite(*this, bytes_written);
                    out_queue.pop_front();
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
                    asio::co_spawn(strand, std::bind(&Connection::Write, shared_from_base()), asio::detached);
                }
            }

            std::shared_ptr<Connection> shared_from_base()
            {
                return std::static_pointer_cast<Connection>(this->shared_from_this());
            }

            bool connect_complete;
            ConnectionProcess& process;
            AwaitableConnection<socket_type> connection;
            asio::strand<typename socket_type::executor_type> strand;
            queue_type out_queue;
        };
    }
}