#ifndef BRILLIANTCLIENTSERVER_CONNECTION_H
#define BRILLIANTCLIENTSERVER_CONNECTION_H

#include <iostream>
#include <stdint.h>
#include "asio.hpp"
#include "brilliant/GeneralConcepts.h"
#include "brilliant/Message.h"
#include "brilliant/Queue.h"
#include "brilliant/Common.h"

//TODO: solve this circular dependency
#include "BrilliantServer.h"

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

		Connection(owner parent, asio::io_context& context, asio::ip::tcp::socket socket, Queue<OwnedMessage<T>>& qIn)
			: iParent(parent), mContext(context), mSocket(std::move(socket)), mInQueue(qIn)
		{
			if (iParent == owner::server)
			{
				mHandshakeOut = std::uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
				mHandshakeCheck = Scramble(mHandshakeOut);
			}
			else
			{
				mHandshakeIn = 0;
				mHandshakeCheck = 0;
			}
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
				if (mSocket.is_open())
				{
					mId = iId;
					WriteValidation();
					ReadValidation(server);
				}
			}
		}

		void ConnectToServer(asio::ip::tcp::resolver::results_type& endpoints)
		{
			if (iParent == owner::client)
			{
				asio::async_connect(mSocket, endpoints,
					[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
						if (!ec)
						{
							ReadValidation<Server<T>>();
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
				asio::post(mContext, [&]() { mSocket.close(); });
			}
		}

		bool IsConnected() const
		{
			return mSocket.is_open();
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
						BCS_LOG(LogLevel::Error) << "[" << mId << "] Read header fail";
						mSocket.close();
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
				BCS_LOG(LogLevel::Error) << "[" << mId << "] Read body fail";
				mSocket.close();
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
						BCS_LOG(LogLevel::Error) << "[" << mId << "] Write header failed";
						mSocket.close();
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
						BCS_LOG(LogLevel::Error) << "[" << mId << "] Write body failed";
						mSocket.close();
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

		std::uint64_t Scramble(std::uint64_t iInput)
		{
			std::uint64_t out = iInput ^ 0xDEADBEEFC0DECAFE;
			out = (out & 0xF0F0F0F0F0F0F0F0) >> 4 | (out & 0xF0F0F0F0F0F0F0F0) << 4;
			return out ^ 0xC0DEFACE12345678;
		}

		void WriteValidation()
		{
			asio::async_write(mSocket, asio::buffer(&mHandshakeOut, sizeof(std::uint64_t)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (iParent == owner::client)
						{
							ReadHeader();
						}
					}
					else
					{
						BCS_LOG(LogLevel::Error) << "Disconnected (WriteValidation): " << ec.message();
						mSocket.close();
					}
				}
			);
		}

		template<class Server>
		void ReadValidation(Server* server = nullptr)
		{
			asio::async_read(mSocket, asio::buffer(&mHandshakeIn, sizeof(std::uint64_t)),
				[this, server](std::error_code ec, std::size_t length) {
					if (!ec)
					{
						if (iParent == owner::server)
						{
							if (mHandshakeIn == mHandshakeCheck)
							{
								BCS_LOG() << "Client Validated";
								server->OnClientValidated(this->shared_from_this());
								ReadHeader();
							}
						}
						else
						{
							mHandshakeOut = Scramble(mHandshakeIn);
							WriteValidation();
						}
					}
					else
					{
						BCS_LOG(LogLevel::Error) << "Client Disconnected (Read Validation) " << ec.message();
						mSocket.close();
					}
				}
			);
		}

		std::uint32_t mId = 0;

		owner iParent = owner::server;

		std::uint64_t mHandshakeOut = 0;
		std::uint64_t mHandshakeIn = 0;
		std::uint64_t mHandshakeCheck = 0;

		asio::ip::tcp::socket mSocket;
		asio::io_context& mContext;

		Message<T> mMsgTmpIn;
		Queue<Message<T>> mOutQueue;
		Queue<OwnedMessage<T>>& mInQueue;
	};
}

#endif