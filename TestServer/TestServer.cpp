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