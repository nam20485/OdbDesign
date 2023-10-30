#include "parse_error.h"
#include <sstream>

namespace Odb::Lib::FileModel
{
    std::string parse_error::buildMessage(const std::string& header) const
    {
        std::stringstream ss;

        ss  << header << std::endl 
            << "location: " << sourceFile.filename().string() << ":" << sourceLine << std::endl
            << "current file:  [" << dataFile.filename().string() << ":" << dataLineNumber << "]" << std::endl
            << "current line:  [" << dataLine << "]" << std::endl
            << "current token: [" << dataToken << "]" << std::endl;

        return ss.str();
    }
}