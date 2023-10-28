#include "OdbFile.h"

namespace Odb::Lib::FileModel
{

    const std::filesystem::path &OdbFile::GetPath() const {
        return m_path;
    }

    bool OdbFile::Parse(std::filesystem::path path)
    {
        m_path = path;
        if (!std::filesystem::exists(m_path)) return false;

        return true;
    }
}