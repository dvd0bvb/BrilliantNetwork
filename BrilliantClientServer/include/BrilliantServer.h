#ifndef BRILLIANTCLIENTSERVER_BRILLIANTSERVER_H
#define BRILLIANTCLIENTSERVER_BRILLIANTSERVER_H

#include <memory>

#include "brilliant/Common.h"
#include "brilliant/Connection.h"
#include "brilliant/Queue.h"
#include "brilliant/Message.h"

namespace Brilliant
{
	template<class T>
	class Server
	{
	public:
		Server(std::uint16_t iPort) : mSslContext(ssl::context::sslv23_server), mAcceptor(mContext, tcp::endpoint(tcp::v4(), iPort))
		{
			mSslContext.set_verify_mode(ssl::context::verify_none);
			//mSslContext.set_default_verify_paths();
		}

		virtual ~Server()
		{
			Stop();
		}

		bool Start()
		{
			try
			{
				WaitForClientConnection();
				mContextThread = std::thread([&]() { mContext.run(); });
			}
			catch (const std::exception& e)
			{
				BCS_LOG(LogLevel::Error) << "[SERVER] Exception: " << e.what();
				return false;
			}

			BCS_LOG() << "Server connected";
			return true;
		}

		void Stop()
		{
			mContext.stop();
			if (mContextThread.joinable())
			{
				mContextThread.join();
			}
			BCS_LOG() << "Server stopped";
		}

		void WaitForClientConnection()
		{
			mAcceptor.async_accept([&](std::error_code ec, tcp::socket socket) {
				if (!ec)
				{
					BCS_LOG() << "[Server] New connection: " << socket.remote_endpoint();
					auto pNewConnection = std::make_shared<Connection<T>>(Connection<T>::owner::server, mContext, ssl::stream<tcp::socket>(std::move(socket), mSslContext), mMsgQueueIn);

					if (OnClientConnect(pNewConnection))
					{
						mConnections.push_back(std::move(pNewConnection));
						mConnections.back()->ConnectToClient(this, iIdCounter++);
						BCS_LOG() << "[" << mConnections.back()->GetId() << "] Connection approved";
					}
					else
					{
						BCS_LOG() << "Connection denied";
					}
				}
				else
				{
					BCS_LOG(LogLevel::Error) << "[Server] New connection error: " << ec.message();
				}

				WaitForClientConnection();
				});
		}

		void MessageClient(std::shared_ptr<Connection<T>> client, const Message<T>& msg)
		{
			if (client && client->IsConnected())
			{
				client->Send(msg);
			}
			else
			{
				OnClientDisconnect(client);
				client.reset();
				mConnections.erase(std::remove(mConnections.begin(), mConnections.end(), client), mConnections.end());
			}
		}

		void MessageAllClients(const Message<T>& msg, std::shared_ptr<Connection<T>> ignore = nullptr)
		{
			bool bInvalidClientExists = false;
			for (auto& client : mConnections)
			{
				if (client && client->IsConnected())
				{
					if (client != ignore)
					{
						client->Send(msg);
					}
				}
				else
				{
					OnClientDisconnect();
					client.reset();
					bInvalidClientExists = true;
				}
			}

			if (bInvalidClientExists)
			{
				mConnections.erase(std::remove(mConnections.begin(), mConnections.end(), nullptr), mConnections.end());
			}
		}

		void Update(std::size_t iMaxMsgs = -1, bool bWait = false)
		{
			if (bWait)
			{
				mMsgQueueIn.Wait();
			}

			std::size_t iNumMsgs = 0;
			while (iNumMsgs < iMaxMsgs && !mMsgQueueIn.Empty())
			{
				auto msg = mMsgQueueIn.PopFront();
				OnMessage(msg.conn, msg.msg);
				iNumMsgs++;
			}
		}

		virtual void OnClientValidated(std::shared_ptr<Connection<T>>  client)
		{

		}

		ssl::context& GetSslContext() noexcept { return mSslContext; }
		const ssl::context& GetSslContext() const noexcept { return mSslContext; }

	protected:
		virtual bool OnClientConnect(std::shared_ptr<Connection<T>> client)
		{
			return false;
		}

		virtual void OnClientDisconnect(std::shared_ptr<Connection<T>> client)
		{
			
		}

		virtual void OnMessage(std::shared_ptr<Connection<T>> client, Message<T>& msg)
		{

		}

		Queue<OwnedMessage<T>> mMsgQueueIn;
		std::deque<std::shared_ptr<Connection<T>>> mConnections;
		asio::io_context mContext;
		ssl::context mSslContext;
		std::thread mContextThread;
		asio::ip::tcp::acceptor mAcceptor;
		std::uint32_t iIdCounter = 10000;
	};
}

#endif