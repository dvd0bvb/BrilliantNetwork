#pragma once

#include "BrilliantServer.h"
#include "TestMsg.h"

namespace
{
	class TestServer : public BrilliantNetwork::Server<TestMessage>
	{
	public:
		TestServer(std::uint16_t iPort) : BrilliantNetwork::Server<TestMessage>(iPort) {}

		bool OnClientConnect(std::shared_ptr<BrilliantNetwork::Connection<TestMessage>> client) override { return true; }

		void OnMessage(std::shared_ptr<BrilliantNetwork::Connection<TestMessage>> client, BrilliantNetwork::Message<TestMessage>& msg) override;
	};
}