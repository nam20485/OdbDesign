#include "StandardFontsFile.h"

#include "Logger.h"

namespace Odb::Lib::FileModel::Design
{
	bool StandardFontsFile::Parse(std::filesystem::path path)
	{
		Utils::Logger::instance()->operator<<("Parsing standard fonts file");

		return true;
	}
}