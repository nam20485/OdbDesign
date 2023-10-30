#pragma once

#include <filesystem>
#include <string>

namespace Odb::Lib::FileModel
{
	struct parse_info
	{
		// data file
		std::filesystem::path dataFile;
		int dataLineNumber;
		std::string dataLine;
		std::string dataToken;

		// source file
		int sourceLine;
		std::filesystem::path sourceFile;

		parse_info(std::filesystem::path dataFile, const std::string& szDataLine, const std::string& szDataToken, int dataLineNumber, int sourceLine, const char* szSourceFile)
			: dataFile(dataFile), dataLine(szDataLine), dataToken(szDataToken), dataLineNumber(dataLineNumber), sourceLine(sourceLine), sourceFile(szSourceFile)			
		{
		}

		parse_info(std::filesystem::path dataFile, const std::string& szDataLine, int dataLineNumber, int sourceLine, const char* szSourceFile)
			: parse_info(dataFile, szDataLine, "", dataLineNumber, sourceLine, szSourceFile)
		{
		}

		std::string toString(const std::string& message = "") const;
	};
}
