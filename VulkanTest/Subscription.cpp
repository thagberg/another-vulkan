#include "Subscription.h"


namespace hvk {

	template <class T, uint16_t N>
	Subscription<T, N>::Subscription(SubscriptionPolicy policy) :
		mPolicy(policy),
		mNotes(),
		mHead(N),
		mTail(0)
	{
	}


	template <class T, uint16_t N>
	Subscription<T, N>::~Subscription()
	{
	}

	template <class T, uint16_t N>
	void Subscription<T, N>::write(T data) {
		mHead++;

		if (mHead == mTail) {
			switch (mPolicy) {
			case SubscriptionPolicy::OverwriteOldest:
				mNotes[mHead] = data;
				break;
			case SubscriptionPolicy::DiscardIncoming:
				mHead--;
				break;
			}
		}

		if (mHead > N) {
			mHead = 0;
		}
	}

	template <class T, uint16_t N>
	void Subscription<T, N>::read(T* dataOut) {
		if (mTail != mHead) {
			dataOut = &mNotes[mTail];
			mTail++;
			if (mTail >= N) {
				mTail = 0;
			}
		}
	}
}
