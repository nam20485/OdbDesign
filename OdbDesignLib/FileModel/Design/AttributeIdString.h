#pragma once
#include <string>
#include <map>


namespace Odb::Lib::FileModel::Design
{
	class AttributeIdString
	{
	public:		

	protected:
		AttributeIdString() = default;	// abtract class
	
		std::map<std::string, std::string> m_attributeAssignments;

		bool ParseAttributeIdString(const std::string& attributeIdString);
	};
}
