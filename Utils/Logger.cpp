#include "Logger.h"


namespace Utils
{
	Logger::Logger()		
		: m_logMessageLoop([&](Message& message)
			{
				return logMessage(message);
			})
		, m_level(Level::Info)
	{
	}

	inline Logger* Logger::instance()
	{
		if (_instance == nullptr)
		{
			_instance = new Logger();
		}
		return _instance;
	}

	/*static*/ Logger* Logger::_instance = nullptr;

	inline Logger::Level Logger::logLevel() const { return m_level; }
	inline void Logger::logLevel(Logger::Level level) { m_level = level; }

	void Logger::log(Level level, const std::string& message, const std::string& file, int line)
	{
		if (level >= m_level)
		{
			Message logMessage
			{
				message,
				level,
				file,
				line
			};

			m_logMessageLoop.addWorkItem(std::move(logMessage));
		}				
	}

	void Logger::error(const std::string& message, const std::string& file, int line)
	{
		log(Level::Error, message, file, line);
	}

	void Logger::warning(const std::string& message, const std::string& file, int line)
	{
		log(Level::Warning, message, file, line);
	}

	void Logger::info(const std::string& message, const std::string& file, int line)
	{
		log(Level::Info, message, file, line);
	}

	void Logger::info(const std::stringstream& stream, const std::string& file, int line)
	{
		info(stream.str(), file, line);
	}

	void Logger::debug(const std::string& message, const std::string& file, int line)
	{
		log(Level::Debug, message, file, line);
	}

	void Logger::exception(const std::exception& e, const std::string& file, int line)
	{
		error(e.what(), file, line);
	}

	void Logger::start()
	{
		m_logMessageLoop.startProcessing();
	}

	void Logger::stop()
	{
		m_logMessageLoop.stopProcessing();
	}

	//bool Logger::processWorkItem(LogMessage& logMessage)
	//{
	//	return this->logMessage(logMessage);
	//}

	std::string Logger::formatLogMessage(const Message& logMessage)
	{
		std::stringstream ss;

		ss << "["
			<< logMessage.timeStamp			
			<< " "
			<< logLevelToString(logMessage.level)			
			<< "]";

		if (logMessage.file != "" &&
			logMessage.line != -1)
		{			
			ss << " "
				<< logMessage.file
				<< ":"
				<< logMessage.line;
		}
		
		ss << " "
			<< logMessage.message
			<< std::endl;

		return ss.str();
	}

	bool Logger::logMessage(const Message& logMessage)
	{
		auto message = formatLogMessage(logMessage);

		std::cout << message;
		//if (logMessage.level >= LogLevel::Warning)
		//{
		//	std::cerr << message;
		//}

		return true;
	}

	std::string Logger::logLevelToString(Level level) const
	{		
		return LogLevelStrings[static_cast<int>(level)];
	}
}