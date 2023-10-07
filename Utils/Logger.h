#pragma once

#include "WorkQueueLoopThread.h"
#include <chrono>


namespace Utils
{
	enum class LogLevel
	{
		None,
		Error,
		Warning,
		Info,
		Debug,
	};

	struct LogMessage
	{
		std::string message;		
		LogLevel level;
		std::chrono::time_point<std::chrono::system_clock> timeStamp;
		std::string file;
		int line;

		LogMessage(std::string message, std::string file, int line, LogLevel level)
			: message(message), file(file), line(line), level(level), timeStamp(std::chrono::system_clock::now())
		{
		}
	};	

	class Logger : public WorkQueueLoopThread<struct LogMessage>
	{
	public:
		Logger();

		LogLevel GetLogLevel() const;
		void SetLogLevel(LogLevel level);

		void Log(LogLevel level, const std::string& message, const std::string& file = "", int line = -1);

	private:
		LogLevel m_level;

		bool processWorkItem(struct LogMessage& logMessage) override;
		std::string formatLogMessage(const struct LogMessage& logMessage);

		std::string logLevelToString(LogLevel level) const;

		const std::string LogLevelStrings[5] = { "None", "Error", "Warning", "Info", "Debug" };

	};
}
