#pragma once

#include "../../odbdesign_export.h"
#include "FeaturesFile.h"
#include "AttrListFile.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT SymbolsDirectory
	{
	public:
		typedef std::vector<std::shared_ptr<SymbolsDirectory>> Vector;
		typedef std::map<std::string, std::shared_ptr<SymbolsDirectory>> StringMap;

	private:

	};
}
