#include <iostream>
#include <queue>
#include <thread>
#include <array>

#include "Connection.h"
#include "ServerProcess.h"
#include "Client.h"
#include "Server.h"

using namespace Brilliant::Network;

class Proc : public Brilliant::Network::ServerProcess
{
public:
    void OnError(ConnectionInterface& connection, const asio::error_code& ec, const std::source_location& location)
    {
        std::cout << ec.value() << ' ' << ec.message() << ' ' << location.function_name() << '\n';
    }

    std::size_t ReadCompletion(ConnectionInterface& connection, const asio::error_code& ec, std::size_t n)
    {
        std::cout << "thread: " << std::this_thread::get_id() << " n: " << n << '\n';
        return sizeof(int) < n ? 0 : (sizeof(int) - n);
    }

    void OnRead(ConnectionInterface& connection, const std::span<std::byte>& data)
    {
        const auto n = data.size();
        if (n >= sizeof(int))
        {
            int i = 0;
            std::memcpy(&i, data.data(), sizeof(int));
            std::cout << "Received message: " << i << '\n';

            int out = 13;
            connection.Send(std::span{ reinterpret_cast<std::byte*>(&out), sizeof(out) });
        }
        else
        {
            std::cout << "Received message size of " << n << '\n';
        }
    }

    void OnWrite(ConnectionInterface& connection, std::size_t n)
    {
        std::cout << "thread: " << std::this_thread::get_id() << " OnWrite: " << n << '\n';
    }

    void OnConnect(ConnectionInterface& connection)
    {
        std::cout << "Connection established\n";
    }

    void OnTimeout(ConnectionInterface& connection)
    {
        std::cout << "Connection timed out\n";
    }

    void OnDisconnect(ConnectionInterface& connection)
    {
        std::cout << "Connection disconnected\n";
    }

    void OnClose(ConnectionInterface& connection)
    {
        std::cout << "Connection closed\n";
    }

    void OnAcceptedConnection(ConnectionInterface& acceptor)
    {
        std::cout << "Connection accepted\n";
    }

    void OnAcceptorError(AcceptorInterface& acceptor, const asio::error_code& ec)
    {
        std::cout << "Acceptor error: " << ec.value() << ' ' << ec.message() << '\n';
    }

    void OnBeginListen(ConnectionInterface& connection)
    {
        std::cout << "Started listening\n";
    }

    void OnAcceptorClose(AcceptorInterface& acceptor)
    {
        std::cout << "Acceptor closed\n";
    }
};

int main()
{
    asio::io_context context;
    Proc proc;

    //server
    std::thread t2([&context, &proc]() {
        std::cout << "server thread " << std::this_thread::get_id() << '\n';
        Brilliant::Network::Server server(context, proc);
        server.AcceptOn<asio::ip::tcp>("8000");
        server.AcceptOn<asio::ip::udp>("8000");

        context.run_for(std::chrono::seconds{2});
        });

    //client
    std::thread t1([&context, &proc]() {
        std::cout << "client thread " << std::this_thread::get_id() << '\n';

        Brilliant::Network::Client<asio::ip::udp> client(context, proc);
        //Brilliant::Network::Client<asio::ip::tcp> client(context, proc);
        client.Connect("127.0.0.1", "8000");
        int out = 1;
        //std::span has no initializer_list ctor yet...
        client.Send(std::span{ &out, sizeof(out) });

        context.run_for(std::chrono::seconds{ 2 });
        });

    if (t2.joinable()) t2.join();
    if (t1.joinable()) t1.join();
}