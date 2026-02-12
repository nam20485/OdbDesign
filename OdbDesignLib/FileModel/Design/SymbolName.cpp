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
		, m_index(-1)
	{
	}

	std::string SymbolName::GetName() const
	{
		return m_name;
	}

	int SymbolName::GetIndex() const
	{
		return m_index;
	}

	UnitType SymbolName::GetUnitType() const
	{
		return m_unitType;
	}

	void SymbolName::ApplyDefaultUnitTypeIfNone(UnitType unitType)
	{
		if (m_unitType == UnitType::None)
		{
			m_unitType = unitType;
		}
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

		if (!token.empty() && token[0] == '$')
		{
			try
			{
				m_index = std::stoi(token.substr(1));
				// Reject negative index values
				if (m_index < 0)
				{
					throw_parse_error(path, line, token, lineNumber);
				}
			}
			catch (const std::invalid_argument&)
			{
				// Surface malformed index tokens instead of silently defaulting.
				throw_parse_error(path, line, token, lineNumber);
			}
			catch (const std::out_of_range&)
			{
				// Surface index tokens that exceed integer range.
				throw_parse_error(path, line, token, lineNumber);
			}
			
			// Get the name token after the index
			if (std::getline(lineStream, token, ' '))
			{
				m_name = token;
			}
		}
		else
		{
			// No index token, use the first token as the name
			m_name = token;
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
		m_index = -1;
	}

	//std::shared_ptr<SymbolName> SymbolName::Parse(const std::string& line)
	//{
	//	return nullptr;
	//}
}
