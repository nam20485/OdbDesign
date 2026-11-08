#include "AttributeLookupTable.h"
#include <sstream>
#include <charconv>

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
					// strict numeric parse: std::from_chars rejects partial values
					// ("5abc", "0x10" -- std::stoul would have accepted their numeric
					// prefixes), negative values ("-5" wraps under std::stoul on
					// platforms with a 32-bit unsigned long) and out-of-range values
					// alike; a rejected value leaves the id unset rather than failing
					// the whole record parse. An empty value ("ID=") never reaches
					// here: the getline guard above fails the parse like any other
					// malformed name=value pair, matching the attribute section's
					// behavior.
					unsigned int idValue = 0;
					const auto* const valueEnd = value.data() + value.size();
					const auto result = std::from_chars(value.data(), valueEnd, idValue);
					if (result.ec == std::errc() && result.ptr == valueEnd)
					{
						parsedId = idValue;
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