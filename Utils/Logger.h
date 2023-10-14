#pragma once

#include "WorkQueueLoopThread.h"
#include <chrono>
#include "utils_export.h"


namespace Utils
{	
	class UTILS_EXPORT Logger// : public WorkQueueLoopThread<struct LogMessage>
	{
	public:
		enum class Level
		{
			None,
			Debug,
			Info,
			Warning,
			Error,
		};

		struct Message
		{
			std::string message;
			Level level;
			std::chrono::time_point<std::chrono::system_clock> timeStamp;
			std::string file;
			int line;

			Message(std::string message, Level level)
				: message(message), level(level), timeStamp(std::chrono::system_clock::now()), file(""), line(-1)
			{
			}

			Message(std::string message, Level level, std::string file, int line)
				: message(message), level(level), timeStamp(std::chrono::system_clock::now()), file(file), line(line)
			{
			}
		};

		Logger();

		static Logger* instance();

		Level logLevel() const;
		void logLevel(Level level);

		void start();
		void stop();

		void log(Level level, const std::string& message, const std::string& file = "", int line = -1);
		void error(const std::string& message, const std::string& file = "", int line = -1);
		void warning(const std::string& message, const std::string& file = "", int line = -1);
		void info(const std::string& message, const std::string& file = "", int line = -1);
		void info(const std::stringstream& message, const std::string& file = "", int line = -1);
		void debug(const std::string& message, const std::string& file = "", int line = -1);
		void exception(const std::exception& e, const std::string& file = "", int line = -1);

		template<class T>
		Logger& operator<<(const T& output);

	private:
		Level m_level;

		//bool processWorkItem(struct LogMessage& logMessage) override;
		std::string formatLogMessage(const struct Message& logMessage);
		bool logMessage(const struct Message& logMessage);

		WorkQueueLoopThread<struct Message> m_logMessageLoop;

		std::string logLevelToString(Level level) const;

		const std::string LogLevelStrings[5] = { "None", "Debug", "Info", "WARNING", "ERROR" };

		static Logger* _instance;
	};	

	template<class T>
	inline Logger& Logger::operator<<(const T& output)
	{
		log(Level::Info, output);
		return *this;
	}
}
