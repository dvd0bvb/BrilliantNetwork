#ifndef BRILLIANTCLIENTSERVER_CONNECTION_H
#define BRILLIANTCLIENTSERVER_CONNECTION_H

#include <cstdint>

#include "brilliant/Common.h"
#include "brilliant/GeneralConcepts.h"
#include "brilliant/Message.h"
#include "brilliant/Queue.h"

namespace Brilliant
{
	template<class T> class Server;

	template<class T>
	class Connection : public std::enable_shared_from_this<Connection<T>>
	{
	public:
		enum class owner
		{
			server,
			client
		};

		Connection(owner parent, asio::io_context& context, ssl::stream<tcp::socket> socket, Queue<OwnedMessage<T>>& qIn)
			: iParent(parent), mContext(context), mSocket(std::move(socket)), mInQueue(qIn)
		{
			mSocket.set_verify_mode(ssl::verify_none);
		}

		owner GetOwner() const { return iParent; }

		auto GetId() const { return mId; }

		virtual ~Connection()
		{

		}

		void ConnectToClient(Server<T>* server, std::uint32_t iId = 0)
		{
			if (iParent == owner::server)
			{
				if (mSocket.lowest_layer().is_open())
				{
					mId = iId;
					//WriteValidation();
					//ReadValidation(server);
					mSocket.async_handshake(mSocket.server, [this, server](const std::error_code& ec) {
						if (!ec)
						{
							BCS_LOG() << "Client Validated";
							server->OnClientValidated(this->shared_from_this());
							ReadHeader();
						}
						else
						{
							BCS_LOG(LogLevel::Error) << "Handshake failed: " << ec.message();
							mSocket.lowest_layer().close();
						}
						});
				}
			}
		}

		void ConnectToServer(asio::ip::tcp::resolver::results_type& endpoints)
		{
			if (iParent == owner::client)
			{
				asio::async_connect(mSocket.lowest_layer(), endpoints,
					[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
						if (!ec)
						{
							//ReadValidation();
							mSocket.async_handshake(mSocket.client, [this](const std::error_code& ec) {
								if (!ec)
								{
									ReadHeader();
								}
								else
								{
									BCS_LOG() << "Handshake failed: " << ec.message();
									mSocket.lowest_layer().close();
								}
								});
						}
						else
						{
							BCS_LOG() << "Failed to connect to server: " << ec.message();
						}
					});
			}
		}

		void Disconnect()
		{
			if (IsConnected())
			{
				asio::post(mContext, [&]() { mSocket.lowest_layer().close(); });
			}
		}

		bool IsConnected() const
		{
			return mSocket.lowest_layer().is_open();
		}

		void Send(const Message<T>& msg)
		{
			asio::post(mContext, [this, msg]() {
				bool bWritingMsg = !mOutQueue.Empty();
				mOutQueue.PushBack(msg);
				if (!bWritingMsg)
				{
					WriteHeader();
				}
				});
		}

	private:
		void ReadHeader()
		{
			asio::async_read(mSocket, asio::buffer(&mMsgTmpIn.mHeader, sizeof(message_header<T>)),
				[this](std::error_code ec, std::size_t iLength) {
					if (!ec)
					{
						if (mMsgTmpIn.mHeader.iSize > 0)
						{
							mMsgTmpIn.mBody.resize(mMsgTmpIn.mHeader.iSize);
							ReadBody();
						}
						else
						{
							AddToIncomingMessageQueue();
						}
					}
					else
					{
						BCS_LOG(LogLevel::Error) << "[" << mId << "] Read header fail. Message: " << ec.message();
						mSocket.lowest_layer().close();
					}
				});
		}

		void ReadBody()
		{
			std::error_code ec;
			mSocket.read_some(asio::buffer(mMsgTmpIn.mBody.data(), mMsgTmpIn.mBody.size()), ec);
			if (!ec)
			{
				AddToIncomingMessageQueue();
			}
			else
			{
				BCS_LOG(LogLevel::Error) << "[" << mId << "] Read body fail. Message: " << ec.message();
				mSocket.lowest_layer().close();
			}
		}

		void WriteHeader()
		{
			asio::async_write(mSocket, asio::buffer(&mOutQueue.Front().mHeader, sizeof(message_header<T>)),
				[this](std::error_code ec, std::size_t iLength) {
					if (!ec)
					{
						if (mOutQueue.Front().mBody.size() > 0)
						{
							WriteBody();
						}
						else
						{
							mOutQueue.PopFront();

							if (!mOutQueue.Empty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						BCS_LOG(LogLevel::Error) << "[" << mId << "] Write header failed. Message: " << ec.message();
						mSocket.lowest_layer().close();
					}
				});
		}

		void WriteBody()
		{
			asio::async_write(mSocket, asio::buffer(mOutQueue.Front().mBody.data(), mOutQueue.Front().mBody.size()),
				[this](std::error_code ec, std::size_t iLength) {
					if (!ec)
					{
						mOutQueue.PopFront();

						if (!mOutQueue.Empty())
						{
							WriteHeader();
						}
					}
					else
					{
						BCS_LOG(LogLevel::Error) << "[" << mId << "] Write body failed. Message: " << ec.message();
						mSocket.lowest_layer().close();
					}
				});
		}

		void AddToIncomingMessageQueue()
		{
			if (iParent == owner::server)
			{
				mInQueue.PushBack({ mMsgTmpIn, this->shared_from_this() });
			}
			else
			{
				mInQueue.PushBack({ mMsgTmpIn, nullptr });
			}

			ReadHeader();
		}

		std::uint32_t mId = 0;
		owner iParent = owner::server;

		ssl::stream<tcp::socket> mSocket;
		asio::io_context& mContext;

		Message<T> mMsgTmpIn;
		Queue<Message<T>> mOutQueue;
		Queue<OwnedMessage<T>>& mInQueue;
	};
}

#endif