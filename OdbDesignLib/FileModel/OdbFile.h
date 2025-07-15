#pragma once

#include <filesystem>
#include "../odbdesign_export.h"

namespace Odb::Lib::FileModel
{
	class ODBDESIGN_EXPORT OdbFile
	{
    public:
        virtual ~OdbFile() = default;

        const std::filesystem::path& GetPath() const;
        const std::filesystem::path& GetDirectory() const;
        const std::string& GetFilename() const;

        virtual bool Parse(std::filesystem::path path);

    protected:
        std::filesystem::path m_path;
        std::filesystem::path m_directory;
        std::string m_filename;

        // abstract class
        OdbFile() = default;

	};
}
