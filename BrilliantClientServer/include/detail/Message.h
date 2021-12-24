#pragma once

#include <vector>
#include <memory>
#include "detail/GeneralConcepts.h"

namespace Brilliant
{
	template<class T>
	struct message_header
	{
		T tId{};
		std::uint32_t iSize = 0;
	};

	template<class T>
	struct Message
	{
		message_header<T> mHeader;
		std::vector<std::uint8_t> mBody;

		std::size_t Size() noexcept
		{
			return sizeof(message_header<T>) + mBody.size();
		}

		//operator<< for simple types
		template<TriviallyCopiable U>
		friend Message<T>& operator<< (Message<T>& msg, U u)
		{
			const auto i = msg.mBody.size();
			msg.mBody.resize(i + sizeof(U));
			std::memcpy(msg.mBody.data() + i, &u, sizeof(U));
			msg.mHeader.iSize = msg.Size();
			return msg;
		}

		friend Message<T>& operator<< (Message<T>& msg, const std::vector<uint8_t>& data)
		{
			const auto i = msg.mBody.size();
			msg.mBody.resize(i + data.size());
			msg.mBody.insert(msg.mBody.end(), data.begin(), data.end());
			msg.mHeader.iSize = msg.Size();
			return msg;
		}

		template<TriviallyCopiable U>
		friend Message<T>& operator>> (Message<T>& msg, U u)
		{
			const auto i = msg.mBody.size() - sizeof(U);
			std::memcpy(&u, msg.mBody.data() + i, sizeof(U));
			msg.mBody.resize(i);
			msg.mHeader.iSize = msg.Size();
			return msg;
		}
	};

	template<class T>
	class Connection;

	template<class T>
	struct OwnedMessage
	{
		Message<T> msg;
		std::shared_ptr<Connection<T>> conn;
	};
}