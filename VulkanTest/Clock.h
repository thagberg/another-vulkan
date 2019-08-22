#pragma once

#include <chrono>

namespace hvk {

	class Clock
	{
	private:
		std::chrono::high_resolution_clock mClock;
		std::chrono::high_resolution_clock::time_point mStartTime;
		std::chrono::high_resolution_clock::time_point mEndTime;
		std::chrono::duration<double>  mDelta;
		bool mStarted;

	public:
		Clock();
		~Clock();

		void start();
		void end();
		double getDelta() { return mDelta.count(); }
	};
}
