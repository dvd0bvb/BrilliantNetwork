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
        co_await connection->ReadInto(req);

        std::cout << "Server received: " << req << '\n';

        if (req.body() == "Bye server")
        {
            std::cout << "Server disconnecting\n";
            break;
        }

        http::response<http::string_body> resp(http::status::ok, req.version());
        resp.body() = "Hello client";
        resp.prepare_payload();

        co_await connection->Send(resp);
    }
}

asio::awaitable<void> Server()
{
    asio::ssl::context ssl(asio::ssl::context::tlsv12);
    ssl.use_certificate_chain_file("public.pem");
    ssl.use_private_key_file("private.pem", asio::ssl::context::pem);

    Brilliant::Network::AwaitableServer<Brilliant::Network::HttpsProtocol> server(co_await asio::this_coro::executor);

    auto accept = server.AcceptOn("8000", ssl);
    auto c = co_await accept.async_resume(asio::use_awaitable);
    auto conn = *c;
    co_await ServerProc(conn);
}

asio::awaitable<void> Client()
{
    asio::ssl::context ssl(asio::ssl::context::tlsv12);
    ssl.load_verify_file("public.pem");

    Brilliant::Network::AwaitableClient<Brilliant::Network::HttpsProtocol> client(co_await asio::this_coro::executor, ssl);
    co_await client.Connect("localhost", "8000");

    http::request<http::string_body> req(http::verb::get, "/", 11);
    req.body() = "Hello server";
    req.prepare_payload();

    co_await client.Send(req);

    http::response<http::string_body> resp;
    co_await client.Read(resp);

    req.body() = "Bye server";
    req.prepare_payload();

    co_await client.Send(resp);

    client.Disconnect();
}

void DoHttpsExample()
{
    asio::thread_pool context;
    asio::co_spawn(context, Server(), asio::detached);
    asio::co_spawn(context, Client(), asio::detached);
    context.join();
}