#include <iostream>
#include <iomanip>

#include "BrilliantNetwork.h"
#include "TcpExample.h"

namespace asio = boost::asio;

static asio::awaitable<void> ServerProc(Brilliant::Network::AwaitableConnection<Brilliant::Network::TcpProtocol>* conn)
{
    while (true)
    {
        std::array<char, 13> buffer{};
        if (auto result = co_await conn->ReadInto(asio::buffer(buffer));
            result.second)
            {
                std::cout << "ERROR: " << result.second.message() << '\n';
                break;
            }
        
        std::string_view message{buffer.data(), buffer.size() - 1};
        std::cout << "Server read: " << std::quoted(message) << '\n';

        if (message == "bye server..")
        {
            std::cout << "Server closing connection\n";
            break;
        }

        if (auto result = co_await conn->Send(asio::buffer("hello client"));
            result.second)
            {
                std::cout << "ERROR: " << result.second.message() << '\n';
                break;
            }

        std::cout << "Server sent: " << std::quoted("hello client") << '\n';
    }
    conn->Disconnect();
    co_return;
}

static asio::awaitable<void> Server()
{
    Brilliant::Network::AwaitableServer<Brilliant::Network::TcpProtocol> server(co_await asio::this_coro::executor);

    auto accept = server.AcceptOn("8000");
    //while (accept)
    {
        //note MSVC will not compile this at time of writing
        auto c = co_await accept.async_resume(asio::use_awaitable);
        auto conn = *c;
        if (conn)
        {
            //asio::co_spawn(co_await asio::this_coro::executor, ServerProc(conn), asio::detached);
            co_await ServerProc(conn);
        }
    }
    co_return;
}

static asio::awaitable<void> Client()
{
    Brilliant::Network::AwaitableClient<Brilliant::Network::TcpProtocol> client(co_await asio::this_coro::executor);
    co_await client.Connect("localhost", "8000");
    co_await client.Send(asio::buffer("hello server"));

    std::array<char, 13> buffer{};
    co_await client.Read(asio::buffer(buffer));

    std::cout << "Client received: " << std::quoted(std::string_view{buffer.data(), buffer.size()}) << '\n';
    co_await client.Send(asio::buffer("bye server.."));
    client.Disconnect();
}

void DoTcpExample()
{
    std::cout << "Tcp Example\n";
    asio::io_context context;
    asio::co_spawn(context, Server(), asio::detached);
    asio::co_spawn(context, Client(), asio::detached);
    context.run();
}