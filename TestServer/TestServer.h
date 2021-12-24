#pragma once

#include "BrilliantServer.h"
#include "TestMsg.h"

namespace
{
	class TestServer : public Brilliant::Server<TestMessage>
	{
	public:
		TestServer(std::uint16_t iPort) : Brilliant::Server<TestMessage>(iPort) {}

		bool OnClientConnect(std::shared_ptr<Brilliant::Connection<TestMessage>> client) override { return true; }

		void OnMessage(std::shared_ptr<Brilliant::Connection<TestMessage>> client, Brilliant::Message<TestMessage>& msg) override;
	};
}