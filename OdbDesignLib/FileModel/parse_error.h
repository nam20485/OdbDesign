#pragma once

#include <exception>
#include <string>
#include <filesystem>

namespace Odb::Lib::FileModel
{

#ifndef throw_parse_error
#   define throw_parse_error(dataFile, dataLine, dataToken, dataLineNumber) throw parse_error(dataFile, dataLine, dataToken, dataLineNumber, __LINE__, __FILE__)
#endif 

	class parse_error : public std::exception
	{
	public:			

		parse_error(const char* szDataFile, const char* szDataLine, const char* szDataToken, int dataLineNumber, int sourceLine, const char* szSourceFile)
			: dataFile(szDataFile), dataLine(szDataLine), dataToken(szDataToken), dataLineNumber(dataLineNumber), sourceLine(sourceLine), sourceFile(szSourceFile)
		{				
		}	

		parse_error(std::filesystem::path dataFile, const std::string& szDataLine, const std::string& szDataToken, int dataLineNumber, int sourceLine, const char* szSourceFile)
			: parse_error(dataFile.string().c_str(), szDataLine.c_str(), szDataToken.c_str(), dataLineNumber, sourceLine, szSourceFile)
		{
		}

		parse_error(const char* szDataFile, const char* szDataLine, const char* szDataToken, int sourceLine, const char* szSourceFile)
			: parse_error(szDataFile, szDataLine, szDataToken, -1, sourceLine, szSourceFile)
		{
		}

		parse_error(const char* szDataFile, const char* szDataLine, int sourceLine, const char* szSourceFile)
			: parse_error(szDataFile, szDataLine, "", -1, sourceLine, szSourceFile)
		{
		}

		parse_error(const char* szDataFile, int sourceLine, const char* szSourceFile)
			: parse_error(szDataFile, "", "", -1, sourceLine, szSourceFile)
		{
		}

		~parse_error()
		{
		}

		std::string buildMessage(const std::string& header) const;

	private:	
		// data file
		std::filesystem::path dataFile;
		int dataLineNumber;
		std::string dataLine;
		std::string dataToken;

		// source file
		int sourceLine;
		std::filesystem::path sourceFile;

	};
}
