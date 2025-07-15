#include "Logger.h"
#include <iostream>
#include "timestamp.h"
#include <filesystem>
#include <typeinfo>

using namespace std::filesystem;


namespace Utils
{
	Logger::Logger()
		: m_level(Level::Info)
		, m_logFilename(DEFAULT_LOG_FILENAME)
		, m_outputTypes(OutputTypes::StdOut | OutputTypes::File)
		//, m_enableInternalLogging(false)
		//, m_logFileStream(m_logFilename, std::ios::out | std::ios::app)
		, m_logMessageLoop(
			[&](Message& message)
			{
				return this->logMessage(message);
			})
	{		
	}

	Logger::~Logger()
	{
		if (m_logFileStream) m_logFileStream.close();
	}

	/*static*/ Logger* Logger::instance()
	{
		if (_instance == nullptr)
		{
			_instance = new Logger();
		}
		return _instance;
	}

	/*static*/ Logger* Logger::_instance = nullptr;

	Logger::Level Logger::logLevel() const { return m_level; }
	void Logger::logLevel(Logger::Level level) { m_level = level; }

	void Logger::outputTypes(int outputTypes)
	{
		m_outputTypes = outputTypes;				
	}

	int Logger::outputTypes() const
	{
		return m_outputTypes;
	}

	std::string Logger::filename() const
	{
		return m_logFilename;
	}

	void Logger::log(Level level, const std::string& message, const std::string& file, int line)
	{
		m_logMessageLoop.addWorkItem(Message { message, level, file, line });						
	}

	void Logger::error(const std::string& message, const std::string& file, int line)
	{
		log(Level::Error, message, file, line);
	}

	void Logger::warn(const std::string& message, const std::string& file, int line)
	{
		log(Level::Warn, message, file, line);
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
		exception(e, "", file, line);
	}

	void Logger::exception(const std::exception& e, const std::string& message, const std::string& file, int line)
	{
		const auto& tid = typeid(e);

		std::stringstream ss;
		ss << "EXCEPTION: "
			<< tid.name() 
			<< ": \""
			<< e.what()
			<< "\"";

		if (!message.empty())
		{
			ss << std::endl << message;
		}

		error(ss.str(), file, line);
	}

	//void Logger::exception(const std::string& message, const std::string& file, int line)
	//{
	//	std::string s = "EXCEPTION!: " + message;
	//	error(s, file, line);
	//}

	void Logger::start()
	{
		loginfo("**************** logger started ****************");
		m_logMessageLoop.startProcessing();
	}

	void Logger::stop()
	{
		loginfo("**************** logger stopped ****************");
		m_logMessageLoop.stopProcessing();
	}

	//bool Logger::processWorkItem(LogMessage& logMessage)
	//{
	//	return this->logMessage(logMessage);
	//}

	/*static*/ std::string Logger::formatLogMessage(const Message& logMessage)
	{
		std::stringstream ss;

		ss << "["
			//<< std::format("", logMessage.timeStamp)
			//<< logMessage.timeStamp.
			<< make_timestamp(logMessage.timeStamp);

		if (logMessage.file != "" &&
			logMessage.line != -1)
		{			
			ss << " " 
				<< std::setw(20)
				<< std::left
				<< path{ logMessage.file }.filename().string()
				<< ":"
				<< std::setw(4)
				<< logMessage.line;
		}
		
		ss << " "
			<< std::setw(6)
			<< std::right
			<< logLevelMap.getValue(logMessage.level)
			<< "]";
		
		ss << " "
			<< logMessage.message
			<< std::endl;

		return ss.str();
	}

	bool Logger::logMessage(const Message& logMessage)
	{
		//std::cout << "[Logger::logMessage] enter, formatting message" << std::endl;

		auto message = formatLogMessage(logMessage);

		//std::cout << "[Logger::logMessage] message formatted, writing to cout and cerr" << std::endl;

		if (logMessage.level >= m_level)
		{
			if (m_outputTypes & OutputTypes::StdOut) std::cout << message << std::flush;
			if (m_outputTypes & OutputTypes::StdErr) std::cerr << message << std::flush;
		}

		//else if (logMessage.level == Level::Error ||
		//		 logMessage.level == Level::Warn)
		//{
		//	if (m_outputTypes & OutputTypes::StdErr) std::cerr << message << std::flush;
		//}

		//std::cout << "[Logger::logMessage] wrote to cout and cerr, writing to file..." << std::endl;

		if (m_outputTypes & OutputTypes::File)
		{
			//std::cout << "[Logger::logMessage] opening log file stream (" << m_logFilename << ")..." << std::endl;

			m_logFileStream.open(m_logFilename, std::ios::out | std::ios::app);
			if (m_logFileStream)
			{
				//std::cout << "[Logger::logMessage] file stream opened, writing to file..." << std::endl;

				m_logFileStream << message << std::flush;
				//m_logFileStream.flush();
				m_logFileStream.close();

				//std::cout << "[Logger::logMessage] wrote to file and closed stream" << std::endl;
			}
		}

		//std::cout << "[Logger::logMessage] exit" << std::endl;

		return true;
	}	
}