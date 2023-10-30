#include "parse_error.h"

namespace Odb::Lib::FileModel
{

    std::string parse_error::toString(const std::string& message) const
    {
        return m_parseInfo.toString(message);
    }

    const parse_info& parse_error::getParseInfo() const
    {
        return m_parseInfo;
    }
}
