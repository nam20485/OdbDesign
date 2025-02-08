#include "parse_info.h"
#include <sstream>

namespace Odb::Lib::FileModel
{
	std::string parse_info::toString(const std::string& message /*= ""*/) const
	{
        std::stringstream ss;

        if (!message.empty())
        {
            ss << message << std::endl;
        }

        if (!dataFile.empty() || !dataLine.empty() || !dataToken.empty())
        {
            ss << "current file:  [" << dataFile.relative_path().string() << ":" << dataLineNumber << "]" << std::endl
                << "current line:  [" << dataLine << "]" << std::endl
                << "current token: [" << dataToken << "]";
        }

        return ss.str();
	}
}