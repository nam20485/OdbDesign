#include "AttributeLookupTable.h"
#include <sstream>
#include <limits>

namespace Odb::Lib::FileModel::Design
{
	const std::map<std::string, std::string>& AttributeLookupTable::GetAttributeLookupTable() const
	{
		return m_attributeLookupTable;
	}

	bool AttributeLookupTable::ParseAttributeLookupTable(const std::string& attributeLookupTableString, std::optional<unsigned int>& parsedId)
	{
		parsedId.reset();
		std::stringstream ss(attributeLookupTableString);
		std::string token;

		// skip the content before the first semicolon
		// a failed read here means the attribute string is absent entirely: the
		// attribute field is optional on ODB++ records, so that is not an error
		if (!std::getline(ss, token, ';'))
			return true;

		// attributes
		if (std::getline(ss, token, ';'))
		{
			std::stringstream attributesStream(token);
			std::string attributeAssignment;
			while (std::getline(attributesStream, attributeAssignment, ','))
			{
				if (!attributeAssignment.empty())
				{
					std::string name;
					std::string value;

					std::stringstream aa_ss(attributeAssignment);
					if (attributeAssignment.find("=") != std::string::npos)
					{
						if (!std::getline(aa_ss, name, '=')) return false;
						if (!std::getline(aa_ss, value)) return false;
					}
					else
					{
						if (!std::getline(aa_ss, name)) return false;
					}

					m_attributeLookupTable[name] = value;
				}
			}
		}

		// third section: the record unique identifier ("ID=<uid>")
		// The ODB++ id is a product-model-wide UID, not an attribute: it is returned to
		// the caller via parsedId instead of being inserted into the attribute map. The
		// literal "ID" key is deliberately gone from the map from here on -- the UID
		// lives in the typed `id` field of the owning record (decision D3, 2026-09-15).
		if (std::getline(ss, token, ';'))
		{
			std::string name;
			std::string value;

			std::stringstream token_ss(token);
			if (token.find("=") != std::string::npos)
			{
				if (!std::getline(token_ss, name, '=')) return false;
				if (!std::getline(token_ss, value)) return false;

				if (name == "ID")
				{
					try
					{
						const auto idValue = std::stoul(value);
						if (idValue <= std::numeric_limits<unsigned int>::max())
						{
							parsedId = static_cast<unsigned int>(idValue);
						}
					}
					catch (const std::exception&)
					{
						// non-numeric id value: leave unset rather than fail the whole parse
					}
				}
				else
				{
					// out-of-spec third-section name: keep it as an attribute rather
					// than silently dropping data
					m_attributeLookupTable[name] = value;
				}
			}
		}

		return true;
	}
}