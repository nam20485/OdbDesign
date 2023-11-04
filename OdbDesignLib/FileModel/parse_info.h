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

		parse_info(std::filesystem::path dataFile, const std::string& szDataLine, const std::string& szDataToken, int dataLineNumber)
			: dataFile(dataFile), dataLineNumber(dataLineNumber), dataLine(szDataLine), dataToken(szDataToken)
		{
		}

		parse_info(std::filesystem::path dataFile, const std::string& szDataLine, int dataLineNumber)
			: parse_info(dataFile, szDataLine, "", dataLineNumber)
		{
		}

		std::string toString(const std::string& message = "") const;
	};
}
