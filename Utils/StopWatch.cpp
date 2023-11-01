#include "StopWatch.h"
#include <sstream>
#include <iomanip>

using namespace std::chrono;

namespace Utils
{	
	StopWatch::StopWatch(bool start)
		: m_stopped(false)
	{
		if (start)
		{
			this->start();
		}
	}

	void StopWatch::start()
	{
		m_stopped = false;
		m_started = system_clock::now();
	}

	void StopWatch::stop()
	{		
		m_finished = system_clock::now();
		m_stopped = true;
	}	

	long long StopWatch::getElapsedMilliseconds() const
	{		
		return duration_cast<milliseconds>(getElapsedDuration()).count();		
	}

	double StopWatch::getElapsedSeconds() const
	{
		return getElapsedMilliseconds() / MS_PER_SECOND;
	}

	std::string StopWatch::getElapsedSecondsString(const std::string& suffix /*= ""*/) const
	{
		std::stringstream ss;
		ss  << std::fixed << std::setprecision(3) << getElapsedSeconds() << suffix;
		return ss.str();
	}	

	system_clock::duration StopWatch::getElapsedDuration() const
	{		
		// if stop() hasn't been called yet, use the current time
		if (m_stopped) return m_finished - m_started;
		else return system_clock::now() - m_started;
	}
}