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

	std::string SymbolName::GetName() const
	{
		return m_name;
	}

	UnitType SymbolName::GetUnitType() const
	{
		return m_unitType;
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

	std::unique_ptr<Odb::Lib::Protobuf::SymbolName> SymbolName::to_protobuf() const
	{
		auto message = std::make_unique<Odb::Lib::Protobuf::SymbolName>();
		message->set_name(m_name);
		message->set_unittype(static_cast<Odb::Lib::Protobuf::UnitType>(m_unitType));
		return message;
	}

	void SymbolName::from_protobuf(const Odb::Lib::Protobuf::SymbolName& message)
	{
		m_name = message.name();
		m_unitType = static_cast<UnitType>(message.unittype());
	}

	//std::shared_ptr<SymbolName> SymbolName::Parse(const std::string& line)
	//{
	//	return nullptr;
	//}
}