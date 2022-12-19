#include <iostream>
#include <queue>
#include <thread>

#include <filesystem>
#include <fstream>

#include <BrilliantNetwork.h>

using namespace Brilliant::Network;

class Proc : public Brilliant::Network::ServerProcess
{
public:
    void OnError(ConnectionInterface& connection, const Brilliant::Network::error_code& ec, const std::source_location& location)
    {
        std::cout << ec.value() << ' ' << ec.message() << ' ' << location.file_name() << ' ' << location.function_name() << " line: " << location.line() << '\n';
    }

    std::size_t ReadCompletion(ConnectionInterface& connection, const Brilliant::Network::error_code& ec, std::size_t n)
    {
        return sizeof(int) < n ? 0 : (sizeof(int) - n);
    }

    void OnRead(ConnectionInterface& connection, const std::span<std::byte>& data)
    {
        const auto n = data.size();
        if (n >= sizeof(int))
        {
            int i = 0;
            std::memcpy(&i, data.data(), sizeof(int));
            std::cout << std::this_thread::get_id() << " received message: " << i << '\n';

            static const int data = 13;
            connection.Send({ &data, sizeof(data) });
        }
        else
        {
            std::cout << std::this_thread::get_id() << " received message size of " << n << '\n';
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

    void OnAcceptorError(AcceptorInterface& acceptor, const Brilliant::Network::error_code& ec)
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

    using protocol = asio::ip::udp;
    //using protocol = asio::ip::tcp;

    //server
    std::thread t2([&context, &proc]() {
        std::cout << "server thread " << std::this_thread::get_id() << '\n';
        Brilliant::Network::Server<protocol> server(context, proc);
        server.AcceptOn("8000");

        context.run_for(std::chrono::seconds{2});
        });

    //client
    std::thread t1([&context, &proc]() {
        std::cout << "client thread " << std::this_thread::get_id() << '\n';

        Brilliant::Network::Client<protocol> client(context, proc);
        client.Connect("127.0.0.1", "8000");
        int i = 1;
        client.Send(asio::buffer(&i, sizeof(i)));

        context.run_for(std::chrono::seconds{ 2 });
        });

    if (t2.joinable()) t2.join();
    if (t1.joinable()) t1.join();
}