#pragma once

#include <exception>
#include <string>
#include <filesystem>
#include "parse_info.h"

namespace Odb::Lib::FileModel
{

#ifndef throw_parse_error
#   define throw_parse_error(dataFile, dataLine, dataToken, dataLineNumber) throw parse_error(dataFile, dataLine, dataToken, dataLineNumber, __LINE__, __FILE__)
#endif 

	class parse_error : public std::exception
	{
	public:		

		parse_error(std::filesystem::path dataFile, const std::string& szDataLine, const std::string& szDataToken, int dataLineNumber, int sourceLine, const char* szSourceFile)
			: m_parseInfo(dataFile, szDataLine, szDataToken, dataLineNumber), sourceLine(sourceLine), sourceFile(szSourceFile)
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

		std::string toString(const std::string& message = "") const;

		const parse_info& getParseInfo() const;

		[[nodiscard]] char const* what() const noexcept;

	private:
		// data file
		const parse_info m_parseInfo;

		// source file
		int sourceLine;
		std::filesystem::path sourceFile;

		constexpr inline static const char WHAT_STR[] = "Parse error";

	};
}
