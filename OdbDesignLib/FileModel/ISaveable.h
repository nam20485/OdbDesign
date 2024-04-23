#pragma once

#include <filesystem>
#include "../odbdesign_export.h"

namespace Odb::Lib::FileModel
{
	class ISaveable
	{
	public:
		virtual bool Save(const std::filesystem::path& directory) = 0;

	protected:
		ISaveable() = default;
	};
}