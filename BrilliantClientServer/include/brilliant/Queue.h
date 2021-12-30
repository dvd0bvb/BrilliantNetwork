#ifndef BRILLIANTCLIENTSERVER_QUEUE_H
#define BRILLIANTCLIENTSERVER_QUEUE_H

#include <deque>
#include <mutex>

namespace Brilliant
{
	template<class T>
	class Queue
	{
	public:
		Queue() = default;
		virtual ~Queue() { Clear(); }
		Queue(const Queue&) = delete;		
		Queue& operator= (const Queue&) = delete;\

		auto& Front() noexcept
		{
			std::scoped_lock lock(mQueueMutex);
			return mQueue.front();
		}

		auto& Back() noexcept
		{
			std::scoped_lock lock(mQueueMutex);
			return mQueue.back();
		}

		void PushBack(const T& t)
		{
			std::scoped_lock lock(mQueueMutex);
			mQueue.push_back(t);

			std::unique_lock<std::mutex> ul(mBlockingMutex);
			mBlocking.notify_one();
		}

		void PushFront(T&& t)
		{
			std::scoped_lock lock(mQueueMutex);
			mQueue.push_front(std::forward<T>(t));

			std::unique_lock<std::mutex> ul(mBlockingMutex);
			mBlocking.notify_one();
		}

		auto Empty() noexcept
		{
			std::scoped_lock lock(mQueueMutex);
			return mQueue.empty();
		}

		auto Count() noexcept
		{
			std::scoped_lock lock(mQueueMutex);
			return mQueue.size();
		}


		auto Clear() noexcept
		{
			std::scoped_lock lock(mQueueMutex);
			mQueue.clear();
		}

		auto PopFront() noexcept
		{
			std::scoped_lock lock(mQueueMutex);
			auto t = std::move(mQueue.front());
			mQueue.pop_front();
			return t;
		}

		auto PopBack() noexcept
		{
			std::scoped_lock lock(mQueueMutex);
			auto t = std::move(mQueue.back());
			mQueue.pop_back();
			return t;
		}

		auto Wait()
		{
			while (Empty())
			{
				std::unique_lock<std::mutex> ul(mBlockingMutex);
				mBlocking.wait(ul);
			}
		}

	private:
		std::mutex mQueueMutex;
		std::deque<T> mQueue;

		std::condition_variable mBlocking;
		std::mutex mBlockingMutex;
	};
}

#endif