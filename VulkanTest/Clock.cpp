#include "Clock.h"
#include <cassert>

namespace hvk {

	Clock::Clock():
		mClock(),
		mStarted(false),
		mDelta()
	{
	}


	Clock::~Clock()
	{
	}

	void Clock::start() {
		mStartTime = mClock.now();
		mStarted = true;
	}

	void Clock::end() {
		assert(mStarted);
		mStarted = false;
		mEndTime = mClock.now();
		mDelta = std::chrono::duration_cast<std::chrono::nanoseconds>(mEndTime - mStartTime);
	}
}
