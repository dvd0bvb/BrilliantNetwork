#ifndef BRILLIANTCLIENTSERVER_BRILLIANTCLIENT_H
#define BRILLIANTCLIENTSERVER_BRILLIANTCLIENT_H

#include "brilliant/Common.h"
#include "brilliant/Queue.h"
#include "brilliant/Message.h"
#include "brilliant/Connection.h"

namespace Brilliant
{
	template<class T>
	class Client
	{
	public:
		Client() : mSslContext(ssl::context::sslv23_client), mSocket(mContext, mSslContext)
		{
			mSslContext.set_verify_mode(ssl::context::verify_none);
			mSocket.set_verify_mode(ssl::verify_none);
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

				mConnection = std::make_unique<Connection<T>>(Connection<T>::owner::client, mContext, ssl::stream<tcp::socket>(mContext, mSslContext), qMessagesIn);
				mConnection->ConnectToServer(endpoint);
				mContextThread = std::thread([this]() { mContext.run(); });
				return true;
			}
			catch (const std::exception& e)
			{
				BCS_LOG(LogLevel::Error) << "Error:" << e.what();
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

		ssl::context& GetSslContext() noexcept { return mSslContext; }
		const ssl::context& GetSslContext() const noexcept { return mSslContext; }

	protected:
		asio::io_context mContext;
		ssl::context mSslContext;
		std::thread mContextThread;
		ssl::stream<tcp::socket> mSocket;
		std::unique_ptr<Connection<T>> mConnection;

	private:
		Queue<OwnedMessage<T>> qMessagesIn;
	};
}

#endif