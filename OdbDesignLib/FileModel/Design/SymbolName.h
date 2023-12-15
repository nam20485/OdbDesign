#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include "../../enums.h"
#include "../../odbdesign_export.h"
#include "../parse_error.h"
#include "../../IProtoBuffable.h"

// TODO: add SymbolName serialization

namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT SymbolName 
	{
	public:
		SymbolName();		

		bool Parse(const std::filesystem::path& path, const std::string& line, int lineNumber);

		typedef std::vector<std::shared_ptr<SymbolName>> Vector;

	private:
		std::string m_name;
		UnitType m_unitType;

	};
}
