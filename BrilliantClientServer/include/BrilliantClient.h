#pragma once

#include <iostream>
#include "asio.hpp"
#include "detail/Queue.h"
#include "detail/Message.h"
#include "detail/Connection.h"

namespace BrilliantNetwork
{
	template<class T>
	class Client
	{
	public:
		Client() : mSocket(mContext)
		{

		}

		virtual ~Client()
		{
			Disconnect();
		}

		bool Connect(const std::string& sHost, std::uint16_t iPort) noexcept
		{
			try
			{
				asio::ip::tcp::resolver resolver(mContext);
				auto endpoint = resolver.resolve(sHost, std::to_string(iPort));

				mConnection = std::make_unique<Connection<T>>(Connection<T>::owner::client, mContext, asio::ip::tcp::socket(mContext), qMessagesIn);
				mConnection->ConnectToServer(endpoint);
				mContextThread = std::thread([this]() { mContext.run(); });
				return true;
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error:" << e.what() << '\n';
				return false;
			}
		}

		void Disconnect() noexcept
		{
			if (mConnection)
			{
				mConnection->Disconnect();
			}

			mContext.stop();
			if (mContextThread.joinable())
			{
				mContextThread.join();
			}

			mConnection.release();
		}

		bool IsConnected() noexcept
		{
			if (mConnection)
			{
				return mConnection->IsConnected();
			}
			return false;
		}

		auto& GetIncoming() noexcept
		{
			return qMessagesIn;
		}

	protected:
		asio::io_context mContext;
		std::thread mContextThread;
		asio::ip::tcp::socket mSocket;
		std::unique_ptr<Connection<T>> mConnection;

	private:
		Queue<OwnedMessage<T>> qMessagesIn;
	};
}