#include "parse_error.h"
#include <sstream>

namespace Odb::Lib::FileModel
{

    std::string parse_error::toString(const std::string& message) const
    {
        std::stringstream ss;
        ss << m_parseInfo.toString(message) << std::endl;
        ss << "location: " << sourceFile.filename().string() << ":" << sourceLine;
        return ss.str();
    }

    const parse_info& parse_error::getParseInfo() const
    {
        return m_parseInfo;
    }

    char const* parse_error::what() const noexcept
    {
        return WHAT_STR;
    }
}
