#include <iostream>
#include <vector>
#include <string_view>

#include "BrilliantNetwork.h"

asio::awaitable<void> ServerProc(Brilliant::Network::AwaitableConnectionInterface* conn)
{
    while (true)
    {
        std::vector<char> message(sizeof("hello server"));
        co_await Brilliant::Network::ReadInto(*conn, message);

        std::cout << "Server read: " << message.data() << '\n';

        if (std::string_view(message.begin(), message.end()) == "bye server..")
        {
            std::cout << "Server closing connection\n";
            conn->Disconnect();
            co_return;
        }

        co_await Brilliant::Network::Send(*conn, "hello client");

        std::cout << "Server sent: " << std::quoted("hello client") << '\n';
    }
}

asio::awaitable<void> Server(asio::io_context& context)
{
    Brilliant::Network::AwaitableServer server(context);
    auto accept = server.AcceptOn<asio::ip::tcp>("8000");
    while (accept)
    {
        //note MSVC will not compile this at time of writing
        if (auto conn = co_await accept.async_resume(asio::use_awaitable); conn && *conn)
        {
            asio::co_spawn(context, ServerProc(*conn), asio::deferred);
        }
    }
    co_return;
}

asio::awaitable<void> Client(asio::io_context& context)
{
    Brilliant::Network::AwaitableClient<asio::ip::tcp> client(context);
    co_await client.Connect("localhost", "8000");
    co_await client.Send("hello server");

    std::vector<char> buffer(sizeof("hello client"));
    co_await client.Read(buffer);

    std::cout << "Client received: " << std::string_view(buffer.data(), buffer.size()) << '\n';
    co_await client.Send("bye server..");
    client.Disconnect();
}

int main(int argc, char* argv[])
{
    asio::io_context context;

    asio::co_spawn(context, Server(context), asio::detached);
    asio::co_spawn(context, Client(context), asio::detached);

    context.run();
}