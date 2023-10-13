#include "Logger.h"


namespace Utils
{
	Logger::Logger()
		: m_level(LogLevel::Info)
	{
	}

	inline LogLevel Logger::GetLogLevel() const { return m_level; }
	inline void Logger::SetLogLevel(LogLevel level) { m_level = level; }

	void Logger::Log(LogLevel level, const std::string& message, const std::string& file, int line)
	{
	}

	bool Logger::processWorkItem(LogMessage& logMessage)
	{
		auto message = formatLogMessage(logMessage);

		std::cout << message << std::endl;
		if (logMessage.level <= LogLevel::Warning)
		{
			std::cerr << message << std::endl;
		}		

		return true;
	}

	std::string Logger::formatLogMessage(const LogMessage& logMessage)
	{
		std::stringstream ss;

		ss << "["
			<< logMessage.timeStamp.time_since_epoch().count()
			<< "]"
			<< " "
			<< logLevelToString(logMessage.level)
			<< " "
			<< "-";

		if (logMessage.file != "" &&
			logMessage.line != -1)
		{			
			ss << " "
				<< logMessage.file
				<< ":"
				<< logMessage.line;
		}
		
		ss << " " 
			<< logMessage.message;

		return ss.str();
	}

	std::string Logger::logLevelToString(LogLevel level) const
	{		
		return LogLevelStrings[static_cast<int>(level)];
	}
}