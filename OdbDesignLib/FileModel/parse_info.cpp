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
        
        ss  << "current file:  [" << dataFile.filename().string() << ":" << dataLineNumber << "]" << std::endl
            << "current line:  [" << dataLine << "]" << std::endl
            << "current token: [" << dataToken << "]" << std::endl;

        return ss.str();
	}
}