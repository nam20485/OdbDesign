#pragma once
#include <string>
#include <map>


namespace Odb::Lib::FileModel::Design
{
	class AttributeLookupTable
	{
	public:				

		const std::map<std::string, std::string>& GetAttributeLookupTable() const;
		bool ParseAttributeLookupTable(const std::string& AttributeLookupTableString);

	protected:
		AttributeLookupTable() = default;	// abtract class

		std::map<std::string, std::string> m_attributeLookupTable;
		
	};
}
