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

        virtual bool Parse(std::filesystem::path path);

    protected:
        std::filesystem::path m_path;

        // abstract class
        OdbFile() = default;

	};
}
