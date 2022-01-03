//#define SSL_TEST
#ifdef SSL_TEST
#include <iostream>

#include <asio.hpp>
#include <asio/ssl.hpp>

int main()
{
	namespace ssl = asio::ssl;
	using tcp = asio::ip::tcp;
	static const std::string pk_file{ "../../rootCAKey.pem" };
	static const std::string pem_file{ "../../rootCACert.pem" };

	static constexpr short port = 60000;

	asio::io_context ctx;
	std::thread ctx_thread{ [&ctx]() { ctx.run(); } };

	ssl::context ssl_ctx(ssl::context::sslv23_server);
	ssl_ctx.set_options(ssl::context::default_workarounds | ssl::context::sslv23 | ssl::context::no_sslv2);
	ssl_ctx.use_certificate_chain_file(pem_file);
	ssl_ctx.use_private_key_file(pk_file, ssl::context::pem);

	tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), port));
	auto socket = acceptor.accept();
	ssl::stream<decltype(socket)> ssl_stream(std::move(socket), ssl_ctx);

	std::error_code ec;
	ssl_stream.handshake(ssl::stream_base::server, ec);
	if (!ec)
	{
		std::cout << "Do read and write stuff\n";
	}
	else
	{
		std::cout << "Handshake error: " << ec.message() << '\n';
	}

	if (ctx_thread.joinable())
		ctx_thread.join();

	std::getchar();
}
#else

#include "TestServer.h"

int main()
{
	try
	{
		namespace ssl = asio::ssl;

		static const std::string pk_file{ "../../rootCAKey.pem" };
		static const std::string pem_file{ "../../rootCACert.pem" };

		Brilliant::InitLog<Brilliant::iClientServerLogId>();

		TestServer server(60000);
		server.Start();
		auto& ssl_ctx = server.GetSslContext();
		ssl_ctx.set_options(ssl::context::default_workarounds | ssl::context::sslv23 | ssl::context::no_sslv2);
		ssl_ctx.use_certificate_chain_file(pem_file);
		ssl_ctx.use_private_key_file(pk_file, ssl::context::pem);

		while (1)
		{
			server.Update(-1, true);
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		Brilliant::Log<Brilliant::iClientServerLogId>(Brilliant::LogLevel::Error) << "ERROR: " << e.what();
		return 1;
	}
}

void TestServer::OnMessage(std::shared_ptr<Brilliant::Connection<TestMessage>> client, Brilliant::Message<TestMessage>& msg)
{
	switch (msg.mHeader.tId)
	{
	case TestMessage::ServerPing:
		std::cout << '[' << client->GetId() << "]: Server Ping\n";
		MessageClient(client, msg);
		break;
	}
}
#endif