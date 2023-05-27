#include <iostream>
#include "BrilliantNetwork.h"
#include "HttpExample.h"

namespace asio = boost::asio;
namespace http = boost::beast::http;

asio::awaitable<void> ServerProc(Brilliant::Network::AwaitableConnection<Brilliant::Network::HttpProtocol>* conn)
{
    co_await conn->Connect();
    while(true)
    {
        http::request<http::string_body> req;
        if (auto [_, ec] = co_await conn->ReadInto(req); ec) 
        {
            std::cout << "Server error: " << ec.message() << '\n';
            break;
        } 
        std::cout << "Server read: " << req;

        if (req.body() == "Bye server")
        {
            break;
        }

        http::response<http::string_body> resp(http::status::ok, req.version());
        resp.body() = "Hello client";
        resp.prepare_payload();
        if (auto [_, ec] = co_await conn->Send(resp); ec)
        {
            std::cout << "Server error: " << ec.message() << '\n';
        }
    }
    std::cout << "Disconnecting server\n";
    conn->Disconnect();
    co_return;
}

asio::awaitable<void> Server()
{
    Brilliant::Network::AwaitableServer<Brilliant::Network::HttpProtocol> server(co_await asio::this_coro::executor);
    auto acceptor = server.AcceptOn("8000");

    auto c = co_await acceptor.async_resume(asio::use_awaitable);
    auto conn = *c;
    co_await ServerProc(conn);
}

asio::awaitable<void> Client()
{
    Brilliant::Network::AwaitableClient<Brilliant::Network::HttpProtocol> client(co_await asio::this_coro::executor);
    if (auto ec = co_await client.Connect("localhost", "8000"); ec)
    {
        std::cout << "Client error: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    http::request<http::string_body> req(http::verb::get, "/", 11);
    req.body() = "Hello server";
    req.prepare_payload();

    if (auto [_, ec] = co_await client.Send(req); ec)
    {
        std::cout << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    http::response<http::string_body> resp;
    if (auto [_, ec] = co_await client.Read(resp); ec)
    {
        std::cout << "Client error: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    std::cout << "Client read: " << resp << '\n';

    req.body() = "Bye server";
    req.prepare_payload();
    if (auto [_, ec] = co_await client.Send(req); ec)
    {
        std::cout << "Client error: " << ec.message() << '\n';
    }

    client.Disconnect();
}

void DoHttpExample()
{
    std::cout << "Http Example\n";

    asio::io_context context;
    asio::co_spawn(context, Server(), asio::detached);
    asio::co_spawn(context, Client(), asio::detached);

    context.run();
}