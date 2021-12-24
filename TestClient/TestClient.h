#pragma once

#include "BrilliantClient.h"
#include "TestMsg.h"

namespace
{
	class TestClient : public Brilliant::Client<TestMessage>
	{
	public:
		void PingServer()
		{
			std::cout << "Hit ping\n";
			Brilliant::Message<TestMessage> msg;
			msg.mHeader.tId = TestMessage::ServerPing;
			msg << std::chrono::system_clock::now();
			mConnection->Send(msg);
		}
	};
}