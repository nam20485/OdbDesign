#include "SymbolName.h"
#include "SymbolName.h"

namespace Odb::Lib::FileModel::Design
{
	//SymbolName::SymbolName(const std::string& name, UnitType unitType)
	//	: m_name(name)
	//	, m_unitType(unitType)
	//{
	//}

	SymbolName::SymbolName()
		: m_name("")
		, m_unitType(UnitType::None)
	{
	}

	bool SymbolName::Parse(const std::filesystem::path& path, const std::string& line, int lineNumber)
	{
		std::stringstream lineStream(line);
		std::string token;

		// $n (index)
		if (!std::getline(lineStream, token, ' '))
		{
			throw_parse_error(path, line, token, lineNumber);
		}

		if (std::getline(lineStream, token, ' '))
		{
			m_name = token;
		}
		else
		{
			throw_parse_error(path, line, token, lineNumber);
		}

		if (std::getline(lineStream, token, ' '))
		{
			switch (token[0])
			{
			case 'M':
				m_unitType = UnitType::Metric;
				break;
			case 'I':
				m_unitType = UnitType::Imperial;
				break;
			}
		}

		return true;
	}

	//std::shared_ptr<SymbolName> SymbolName::Parse(const std::string& line)
	//{
	//	return nullptr;
	//}
}