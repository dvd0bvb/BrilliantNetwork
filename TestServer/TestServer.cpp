#include "TestServer.h"

int main()
{
	TestServer server(60000);
	server.Start();

	while (1)
	{
		server.Update(-1,true);
	}

	return 0;
}

void TestServer::OnMessage(std::shared_ptr<BrilliantNetwork::Connection<TestMessage>> client, BrilliantNetwork::Message<TestMessage>& msg)
{
	switch (msg.mHeader.tId)
	{
	case TestMessage::ServerPing:
		std::cout << '[' << client->GetId() << "]: Server Ping\n";
		MessageClient(client, msg);
		break;
	}
}