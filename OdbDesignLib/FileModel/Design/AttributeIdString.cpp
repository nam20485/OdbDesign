#include "AttributeIdString.h"
#include <sstream>

namespace Odb::Lib::FileModel::Design
{
	bool AttributeIdString::ParseAttributeIdString(const std::string& attributeIdString)
	{
		std::stringstream ss(attributeIdString);
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

					m_attributeAssignments[name] = value;
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

			m_attributeAssignments[name] = value;

		}

		return true;
	}
}