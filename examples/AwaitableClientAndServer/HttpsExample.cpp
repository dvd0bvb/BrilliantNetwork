/**
 * @file HttpsExample.cpp
 * @author David Brill (6david6brill6@gmail.com)
 * 
 * @copyright Copyright (c) 2023
 * Distributed under the Apache License 2.0 (see accompanying
 * file LICENSE or copy at http://www.apache.org/licenses/)
 */

#include <iostream>
#include "BrilliantNetwork.h"
#include "HttpsExample.h"

namespace asio = boost::asio;
namespace http = boost::beast::http;

template<class T>
asio::awaitable<void> ServerProc(T* connection)
{
    co_await connection->Connect();

    while (true)
    {
        http::request<http::string_body> req;
        if (auto [_, ec] = co_await connection->ReadInto(req); ec)
        {
            std::cout << "SERVER ERROR: " << ec.message() << '\n';
            break;
        }

        std::cout << "Server received: " << req << '\n';

        if (req.body() == "Bye server")
        {
            std::cout << "Server disconnecting\n";
            break;
        }

        http::response<http::string_body> resp(http::status::ok, req.version());
        resp.body() = "Hello client";
        resp.prepare_payload();

        if (auto [_, ec] = co_await connection->Send(resp); ec)
        {
            std::cout << "SERVER ERROR: " << ec.message() << '\n';
            break;
        }
    }
}

asio::awaitable<void> HttpsServer()
{
    asio::ssl::context ssl(asio::ssl::context::tlsv12);
    ssl.use_certificate_chain_file("public.pem");
    ssl.use_private_key_file("private.pem", asio::ssl::context::pem);

    Brilliant::Network::AwaitableServer<Brilliant::Network::HttpsProtocol> server(co_await asio::this_coro::executor);
   
    asio::steady_timer timer(co_await asio::this_coro::executor);
    timer.expires_from_now(std::chrono::seconds{3});
    timer.async_wait([&server] (Brilliant::Network::error_code ec) {
        if (ec) { return; }
        std::cout << "Server timed out\n";
        server.Disconnect();
    });
   
    auto accept = server.AcceptOn("8000", ssl);
    auto c = co_await accept.async_resume(asio::use_awaitable);
    auto conn = *c;
    co_await ServerProc(conn);
}

asio::awaitable<void> HttpsClient()
{
    asio::ssl::context ssl(asio::ssl::context::tlsv12);
    ssl.load_verify_file("public.pem");

    Brilliant::Network::AwaitableClient<Brilliant::Network::HttpsProtocol> client(co_await asio::this_coro::executor, ssl);
    if (auto ec = co_await client.Connect("localhost", "8000"); ec)
    {
        std::cout << "CLIENT ERROR: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    http::request<http::string_body> req(http::verb::get, "/", 11);
    req.body() = "Hello server";
    req.prepare_payload();

    if (auto[_, ec] = co_await client.Send(req); ec)
    {
        std::cout << "CLIENT ERROR: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    http::response<http::string_body> resp;
    if (auto[_, ec] = co_await client.Read(resp); ec)
    {
        std::cout << "CLIENT ERROR: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    req.body() = "Bye server";
    req.prepare_payload();

    if (auto[_, ec] = co_await client.Send(resp); ec)
    {
        std::cout << "CLIENT ERROR: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    client.Disconnect();
}

void DoHttpsExample()
{
    std::cout << "Https example\n";
    asio::thread_pool context;
    asio::co_spawn(context, HttpsServer(), asio::detached);
    asio::co_spawn(context, HttpsClient(), asio::detached);
    context.join();
}