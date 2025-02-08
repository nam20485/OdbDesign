#pragma once

#include "WorkQueueLoopThread.h"
#include <chrono>
#include "utils_export.h"
#include <fstream>
#include <string>
#include "EnumMap.h"

namespace Utils
{	

#ifndef loginfo
#	define loginfo(s) Utils::Logger::instance()->info(s, __FILE__, __LINE__)
#endif // loginfo
#ifndef logerror
#	define logerror(s) Utils::Logger::instance()->error(s, __FILE__, __LINE__)
#endif // logerror
#ifndef logwarn
#	define logwarn(s) Utils::Logger::instance()->warn(s, __FILE__, __LINE__)
#endif // logwarn
#ifndef logdebug
#	define logdebug(s) Utils::Logger::instance()->debug(s, __FILE__, __LINE__)
#endif // logdebug
#ifndef logexception
#	define logexception(e) Utils::Logger::instance()->exception(e, __FILE__, __LINE__)
#endif // logexception
#ifndef logexception_msg
#	define logexception_msg(e, m) Utils::Logger::instance()->exception(e, m, __FILE__, __LINE__)
#endif // logexception_msg

	class UTILS_EXPORT Logger final// : public WorkQueueLoopThread<struct LogMessage>
	{
	public:
		enum class Level
		{
			None,
			Debug,
			Info,
			Warn,
			Error,
		};

		struct Message
		{
			std::string message;
			Level level;
			std::chrono::system_clock::time_point timeStamp;
			std::string file;
			int line;

			Message(const std::string& message, Level level)
				: message(message), level(level), timeStamp(std::chrono::system_clock::now()), file(""), line(-1)
			{
			}

			Message(const std::string& message, Level level, const std::string& file, int line)
				: message(message), level(level), timeStamp(std::chrono::system_clock::now()), file(file), line(line)
			{
			}
		};

		enum OutputTypes
		{
			StdOut = 1,
			StdErr = 2,
			File = 4
		};

		Logger();
		~Logger();

		static Logger* instance();

		Level logLevel() const;
		void logLevel(Level level);

		void outputTypes(int outputTypes);
		int outputTypes() const;

		std::string filename() const;

		void start();
		void stop();

		void log(Level level, const std::string& message, const std::string& file = "", int line = -1);
		void error(const std::string& message, const std::string& file = "", int line = -1);
		void warn(const std::string& message, const std::string& file = "", int line = -1);
		void info(const std::string& message, const std::string& file = "", int line = -1);
		void info(const std::stringstream& message, const std::string& file = "", int line = -1);
		void debug(const std::string& message, const std::string& file = "", int line = -1);
		
		void exception(const std::exception& e, const std::string& file = "", int line = -1);
		void exception(const std::exception& e, const std::string& message, const std::string& file = "", int line = -1);
		//void exception(const std::string& message, const std::string& file = "", int line = -1);

		template<class T>
		Logger& operator<<(const T& output);

	private:
		Level m_level;
		std::string m_logFilename;

		int m_outputTypes;
		std::ofstream m_logFileStream;

		//bool m_enableInternalLogging = false;

		WorkQueueLoopThread<struct Message> m_logMessageLoop;		

		//bool processWorkItem(struct LogMessage& logMessage) override;		
		bool logMessage(const struct Message& logMessage);

		static std::string formatLogMessage(const struct Message& logMessage);
		//static std::string logLevelToString(Level level);
			
		static Logger* _instance;	

		inline static constexpr const char DEFAULT_LOG_FILENAME[] = "log.txt";
		//inline static constexpr const char* LogLevelStrings[] = ;

		static const inline EnumMap<Level> logLevelMap{
			{ "NONE", "DEBUG", "INFO", "WARN", "ERROR" }
		};
	};	

	template<class T>
	Logger& Logger::operator<<(const T& output)
	{
		log(Level::Info, output);
		return *this;
	}
}
