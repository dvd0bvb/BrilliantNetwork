//#define SSL_TEST
#ifdef SSL_TEST
#include <iostream>
#include <string_view>

#include <asio.hpp>
#include <asio/ssl.hpp>

int main()
{
	namespace ssl = asio::ssl;
	using tcp = asio::ip::tcp;

	static constexpr auto port = 60000;
	static constexpr std::string_view ip = "127.0.0.1";
	static const std::string pem_file{ "../../rootCACert.pem" };

	asio::io_context ctx;
	std::thread ctx_thread{ [&ctx]() { ctx.run(); } };

	tcp::resolver resolver(ctx);

	ssl::context ssl_ctx(ssl::context::sslv23_client);
	ssl_ctx.set_options(ssl::context::default_workarounds | ssl::context::sslv23 | ssl::context::no_sslv2);
	ssl_ctx.load_verify_file(pem_file);
	ssl_ctx.set_verify_mode(ssl::context::verify_peer);
	
	ssl::stream<tcp::socket> socket(ctx, ssl_ctx);

	asio::connect(socket.lowest_layer(), resolver.resolve(ip, std::to_string(port)));

	std::error_code ec;
	socket.handshake(ssl::stream_base::client, ec);
	if (!ec)
	{
		std::cout << "Do read and write stuff\n";
	}
	else
	{
		std::cerr << "Handshake error: " << ec.message() << '\n';
	}

	if (ctx_thread.joinable())
		ctx_thread.join();

	std::getchar();
}
#else
#include "TestClient.h"

int main()
{
	try
	{
		namespace ssl = asio::ssl;

		static const std::string pem_file{ "../../rootCACert.pem" };

		Brilliant::InitLog<Brilliant::iClientServerLogId>();

		TestClient client;
		client.Connect("127.0.0.1", 60000);
		auto& ssl_ctx = client.GetSslContext();
		ssl_ctx.set_options(ssl::context::default_workarounds | ssl::context::sslv23 | ssl::context::no_sslv2);
		ssl_ctx.load_verify_file(pem_file);
		ssl_ctx.set_verify_mode(ssl::context::verify_peer);

		bool key[3] = { false, false, false };
		bool old_key[3] = { false, false, false };

		bool bQuit = false;
		while (!bQuit)
		{
			if (GetForegroundWindow() == GetConsoleWindow())
			{
				key[0] = GetAsyncKeyState('1') & 0x8000;
				key[1] = GetAsyncKeyState('2') & 0x8000;
				key[2] = GetAsyncKeyState('3') & 0x8000;
			}

			if (key[0] && !old_key[0]) { client.PingServer(); }
			if (key[2] && !old_key[2]) { bQuit = true; }

			for (int i = 0; i < 3; i++)
			{
				old_key[i] = key[i];
			}

			if (client.IsConnected())
			{
				if (!client.GetIncoming().Empty())
				{
					auto msg = client.GetIncoming().PopFront().msg;

					switch (msg.mHeader.tId)
					{
					case TestMessage::ServerPing:
						auto now = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point then;
						msg >> then;
						std::cout << "Ping: " << std::chrono::duration<double>(now - then).count() << '\n';
						break;
					}
				}
			}
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		Brilliant::Log<Brilliant::iClientServerLogId>(Brilliant::LogLevel::Error) << "ERROR: " << e.what();
		return 1;
	}
}
#endif