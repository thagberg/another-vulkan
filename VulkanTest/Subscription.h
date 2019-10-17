#pragma once
#include <array>

namespace hvk {

	enum class SubscriptionPolicy {
		OverwriteOldest,
		DiscardIncoming
	};

	template <class T, uint16_t N>
	class Subscription
	{
	private:
		SubscriptionPolicy mPolicy;
		std::array<T, N> mNotes;
		uint16_t mHead;
		uint16_t mTail;
	public:
		Subscription(SubscriptionPolicy policy=SubscriptionPolicy::DiscardIncoming);
		~Subscription();

		void write(T data);
		void read(T* dataOut);
	};
}
