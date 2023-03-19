#include <iostream>
#include <vector>
#include <string_view>

#include "BrilliantNetwork.h"

namespace asio = boost::asio;

template<class T>
asio::awaitable<void> ServerProc(T* conn)
{
    while (true)
    {
        std::vector<char> message(sizeof("hello server"));
        auto result = co_await conn->ReadInto(asio::buffer(message));
        
        std::cout << result.first << ' ' << result.second << '\n';
        std::cout << "Server read: " << message.data() << '\n';

        if (std::string_view(message.begin(), message.end()) == "bye server..")
        {
            std::cout << "Server closing connection\n";
            conn->Disconnect();
            co_return;
        }

        co_await conn->Send(asio::buffer("hello client"));

        std::cout << "Server sent: " << std::quoted("hello client") << '\n';
    }
}

asio::awaitable<void> Server(asio::io_context& context)
{
    Brilliant::Network::AwaitableServer<Brilliant::Network::UdpProtocol> server(context);
    auto listener = co_await server.AcceptOn("8000");
    listener->Connect();
    co_await ServerProc(listener);

    //auto accept = server.AcceptOn("8000");
    //while (accept)
    //{
    //    //note MSVC will not compile this at time of writing
    //    //if (auto conn = co_await accept.async_resume(asio::use_awaitable); conn && *conn)
    //    //{
    //    //    asio::co_spawn(context, ServerProc(*conn), asio::deferred);
    //    //}
    //}
    co_return;
}

asio::awaitable<void> Client(asio::io_context& context)
{
    Brilliant::Network::AwaitableClient<Brilliant::Network::UdpProtocol> client(context);
    co_await client.Connect("localhost", "8000");
    co_await client.Send(asio::buffer("hello server"));

    std::vector<char> buffer(sizeof("hello client"));
    co_await client.Read(asio::buffer(buffer));

    std::cout << "Client received: " << std::string_view(buffer.data(), buffer.size()) << '\n';
    co_await client.Send(asio::buffer("bye server.."));
    client.Disconnect();
}

int main(int argc, char* argv[])
{
    asio::io_context context;

    asio::co_spawn(context, Server(context), asio::detached);
    asio::co_spawn(context, Client(context), asio::detached);

    //asio::co_spawn(context, [&context]() -> asio::awaitable<void> {
    //    boost::beast::tcp_stream stream(co_await asio::this_coro::executor);

    //    Brilliant::Network::AwaitableConnection<Brilliant::Network::HttpProtocol> conn(std::move(stream));

    //    co_await conn.Connect("localhost", "80");

    //    boost::beast::http::request<boost::beast::http::string_body> req{
    //        boost::beast::http::verb::get, "/", 11
    //    };
    //    req.body() = "Hello from client!";
    //    std::cout << req << '\n';

    //    co_await conn.Send(req);

    //    boost::beast::http::response<boost::beast::http::string_body> response;
    //    co_await conn.ReadInto(response);

    //    //std::cout << response << '\n';

    //    conn.Disconnect();
    //    }, asio::detached);

    //asio::co_spawn(context, [&context]() -> asio::awaitable<void> {
    //    auto acceptor = asio::use_awaitable.as_default_on(asio::ip::tcp::acceptor(co_await asio::this_coro::executor));
    //    asio::ip::tcp::endpoint ep{ asio::ip::tcp::v4(), 80 };
    //    acceptor.open(ep.protocol());
    //    acceptor.bind(ep);
    //    acceptor.listen();

    //    auto socket = co_await acceptor.async_accept();

    //    boost::beast::tcp_stream stream(std::move(socket));

    //    Brilliant::Network::AwaitableConnection<Brilliant::Network::HttpProtocol> conn(std::move(stream));

    //    co_await conn.Connect();

    //    boost::beast::http::request<boost::beast::http::string_body> req;
    //    co_await conn.ReadInto(req);

    //    std::cout << req << '\n';

    //    boost::beast::http::response<boost::beast::http::string_body> response{
    //        boost::beast::http::status::ok, 11
    //    };

    //    response.body() = "Hello from server!";

    //    co_await conn.Send(response);

    //    conn.Disconnect();
    //    }, asio::detached);

    context.run();
}