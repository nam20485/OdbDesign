#include "OdbFile.h"
#include "OdbFile.h"
#include "OdbFile.h"

namespace Odb::Lib::FileModel
{

    const std::filesystem::path &OdbFile::GetPath() const
    {
        return m_path;
    }

    const std::filesystem::path& OdbFile::GetDirectory() const
    {
        return m_directory;
    }

    const std::string& OdbFile::GetFilename() const
    {
        return m_filename;
    }

    bool OdbFile::Parse(std::filesystem::path path)
    {
        m_directory = path;
        if (!std::filesystem::exists(m_directory)) return false;

        return true;
    }
}