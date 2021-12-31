#include "TestServer.h"

int main()
{
	try
	{
		Brilliant::InitLog<Brilliant::iClientServerLogId>();

		TestServer server(60000);
		server.Start();

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