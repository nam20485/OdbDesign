#include "AttributeLookupTable.h"
#include <sstream>

namespace Odb::Lib::FileModel::Design
{
	const std::map<std::string, std::string>& AttributeLookupTable::GetAttributeLookupTable() const
	{
		return m_attributeLookupTable;
	}

	bool AttributeLookupTable::ParseAttributeLookupTable(const std::string& attributeLookupTableString)
	{
		std::stringstream ss(attributeLookupTableString);
		std::string token;

		// attributes
		if (std::getline(ss, token, ';'))
		{
			std::string attributeAssignment;
			while (std::getline(ss, attributeAssignment, ','))
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

		// ID
		if (std::getline(ss, token, ';'))
		{
			std::string name;
			std::string value;

			std::stringstream token_ss(token);
			if (token.find("=") != std::string::npos)
			{
				if (!std::getline(token_ss, name, '=')) return false;
				if (!std::getline(token_ss, value)) return false;
			}

			m_attributeLookupTable[name] = value;
		}

		return true;
	}
}