#pragma once

#include "WorkQueueLoopThread.h"
#include <chrono>
#include "utils_export.h"


namespace Utils
{
	enum class LogLevel
	{
		None,
		Debug,
		Info,		
		Warning,	
		Error,
	};	

	struct LogMessage
	{
		std::string message;		
		LogLevel level;
		std::chrono::time_point<std::chrono::system_clock> timeStamp;
		std::string file;
		int line;

		LogMessage(std::string message, LogLevel level, std::string file, int line)
			: message(message), level(level), timeStamp(std::chrono::system_clock::now()), file(file), line(line)
		{
		}
	};	

	class UTILS_EXPORT Logger// : public WorkQueueLoopThread<struct LogMessage>
	{
	public:
		Logger();

		static Logger* instance();

		LogLevel logLevel() const;
		void logLevel(LogLevel level);

		void start();
		void stop();

		void log(LogLevel level, const std::string& message, const std::string& file = "", int line = -1);
		void error(const std::string& message, const std::string& file = "", int line = -1);
		void warning(const std::string& message, const std::string& file = "", int line = -1);
		void info(const std::string& message, const std::string& file = "", int line = -1);
		void info(const std::stringstream& message, const std::string& file = "", int line = -1);
		void debug(const std::string& message, const std::string& file = "", int line = -1);
		void exception(const std::exception& e, const std::string& file = "", int line = -1);

		template<class T>
		Logger& operator<<(const T& output);

	private:
		LogLevel m_level;

		//bool processWorkItem(struct LogMessage& logMessage) override;
		std::string formatLogMessage(const struct LogMessage& logMessage);
		bool logMessage(const struct LogMessage& logMessage);

		WorkQueueLoopThread<struct LogMessage> m_logMessageLoop;

		std::string logLevelToString(LogLevel level) const;

		const std::string LogLevelStrings[5] = { "None", "Debug", "Info", "WARNING", "ERROR" };

		static Logger* _instance;
	};

	//static Logger g_Logger;

	template<class T>
	inline Logger& Logger::operator<<(const T& output)
	{
		log(LogLevel::Info, output);
		return *this;
	}
}
