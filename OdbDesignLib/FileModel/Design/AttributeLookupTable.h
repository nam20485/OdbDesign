#pragma once
#include <string>
#include <map>
#include "../../odbdesign_export.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT AttributeLookupTable
	{
	public:				

		const std::map<std::string, std::string>& GetAttributeLookupTable() const;
		bool ParseAttributeLookupTable(const std::string& AttributeLookupTableString);

	protected:
		AttributeLookupTable() = default;	// abtract class

		std::map<std::string, std::string> m_attributeLookupTable;
		
	};
}
