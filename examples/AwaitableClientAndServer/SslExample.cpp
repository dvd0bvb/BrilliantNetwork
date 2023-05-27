#include <iostream>
#include <iomanip>
#include <thread>

#include "BrilliantNetwork.h"
#include "SslExample.h"

namespace asio = boost::asio;

static asio::awaitable<void> ServerProc(Brilliant::Network::AwaitableConnection<Brilliant::Network::SslProtocol>* conn)
{
    if (auto errc = co_await conn->Connect(); errc)
    {
        std::cout << "SERVER ERROR: " << errc.message() << '\n';
        conn->Disconnect();
        co_return;
    }

    while (true)
    {
        std::array<char, 13> buffer{};
        if (auto result = co_await conn->ReadInto(asio::buffer(buffer));
            result.second)
            {
                std::cout << "SERVER ERROR: " << result.second.message() << '\n';
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
                std::cout << "SERVER ERROR: " << result.second.message() << '\n';
                break;
            }

        std::cout << "Server sent: " << std::quoted("hello client") << '\n';
    }
    conn->Disconnect();
    co_return;
}

static asio::awaitable<void> Server()
{
    Brilliant::Network::AwaitableServer<Brilliant::Network::SslProtocol> server(co_await asio::this_coro::executor);
    asio::ssl::context ssl(asio::ssl::context::tlsv12);
    ssl.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2);
    ssl.use_certificate_chain_file("public.pem");
    ssl.use_private_key_file("private.pem", asio::ssl::context::pem);
    //ssl.use_tmp_dh_file("dh4096.pem");

    auto accept = server.AcceptOn("9000", ssl);
    //while (accept)
    {
        //connection refused sometimes, add a timeout so program ends gracefully
        asio::steady_timer timer(co_await asio::this_coro::executor);
        timer.expires_from_now(std::chrono::seconds(3));
        timer.async_wait([&server] (Brilliant::Network::error_code ec) {
            if (ec) { return; }
            std::cout << "Server timed out\n";
            server.Disconnect();
        });

        //note MSVC will not compile this at time of writing
        auto c = co_await accept.async_resume(asio::use_awaitable);
        auto conn = *c;
        if (conn)
        {
            //if we got this far, then connection succeeded and we don't need the timeout anymore
            timer.cancel();
            //asio::co_spawn(co_await asio::this_coro::executor, ServerProc(conn), asio::detached);
            co_await ServerProc(conn);
        }
    }
    co_return;
}

static asio::awaitable<void> Client()
{
    asio::ssl::context ssl(asio::ssl::context::tlsv12);
    ssl.load_verify_file("public.pem");

    Brilliant::Network::AwaitableClient<Brilliant::Network::SslProtocol> client(co_await asio::this_coro::executor, ssl);
    //Should we provide the underlying socket or at least a way to access it?
    //socket.set_verify_mode(asio::ssl::verify_peer);
    //socket.set_verify_callback(asio::ssl::host_name_verification("localhost"));
    //ssl context offers these anyway but may be more options we want to set on the socket
    
    if (auto ec = co_await client.Connect("localhost", "9000"); ec)
    {
        std::cout << "CLIENT ERROR: " << ec.message() << '\n'; 
        client.Disconnect();
        co_return;
    }

    if (auto [size, ec] = co_await client.Send(asio::buffer("hello server")); ec)
    {
        std::cout << "CLIENT ERROR: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    std::array<char, 13> buffer{};
    if (auto [size, ec] = co_await client.Read(asio::buffer(buffer)); ec)
    {
        std::cout << "CLIENT ERROR: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    std::cout << "Client received: " << std::quoted(std::string_view{buffer.data(), buffer.size()}) << '\n';
    if (auto [size, ec] = co_await client.Send(asio::buffer("bye server..")); ec)
    {
        std::cout << "CLIENT ERROR: " << ec.message() << '\n';
        client.Disconnect();
        co_return;
    }

    std::cout << "Client sent: \"bye server..\"\n";

    std::this_thread::sleep_for(std::chrono::seconds(1));

    client.Disconnect();
}

void DoSslExample()
{
    std::cout << "Ssl Example\n";
    //Unsure exactly why, but this doesn't work if we use the usual io_context as the ssl_stream.shutdown() call will hang
    //I suspect that it's because of coros running on same thread
    //In normal usage (ie, connecting to a remote peer on another machine) using io_context should be fine
    asio::thread_pool context;

    asio::co_spawn(context, Server(), asio::detached);
    asio::co_spawn(context, Client(), asio::detached);
    
    context.join();
}