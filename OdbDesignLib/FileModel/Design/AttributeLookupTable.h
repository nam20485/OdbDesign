#pragma once
#include <string>
#include <map>
#include <optional>
#include "../../odbdesign_export.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT AttributeLookupTable
	{
	public:				

		const std::map<std::string, std::string>& GetAttributeLookupTable() const;

		// Parses the trailing "<attributes>;ID=<uid>" field of an ODB++ record. The third
		// section is the record's unique identifier, not an attribute: it is returned via
		// parsedId and never inserted into the attribute map, so the UID has exactly one
		// home on the wire (the typed `id` field of the owning record).
		bool ParseAttributeLookupTable(const std::string& attributeLookupTableString, std::optional<unsigned int>& parsedId);

	protected:
		AttributeLookupTable() = default;	// abtract class

		std::map<std::string, std::string> m_attributeLookupTable;
		
	};
}
