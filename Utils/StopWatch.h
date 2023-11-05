#pragma once

#include <chrono>
#include "utils_export.h"
#include <string>

namespace Utils
{
	class UTILS_EXPORT StopWatch
	{
public:
		StopWatch(bool start);		

		void start();
		void stop();

		std::chrono::system_clock::duration getElapsedDuration() const;

		long long getElapsedMilliseconds() const;
		double getElapsedSeconds() const;			

		std::string getElapsedSecondsString(const std::string& suffix = "") const;
		
		//template<class D>
		//std::chrono::system_clock::duration getElapsedDuration() const;			
		
		constexpr inline static const double MS_PER_SECOND = 1000.0;

	private:
		std::chrono::system_clock::time_point m_started;
		std::chrono::system_clock::time_point m_finished;

		bool m_stopped;

	};
}