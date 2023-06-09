#include "Package.h"

namespace Odb::Lib::ProductModel
{
	Package::Package(std::string name, unsigned int index)
		: m_name(name)
		, m_index(index)
	{
	}

	std::string Package::GetName() const
	{
		return m_name;
	}

	//const Pin::Vector& Package::GetPins() const
	//{
	//	return m_pins;
	//}

	unsigned int Package::GetIndex() const
	{
		return m_index;
	}

	void Package::AddPin(std::string name)
	{
		auto index = static_cast<unsigned int>(m_pins.size());
		auto pPin = std::make_shared<Pin>(name, index);
		m_pins.push_back(pPin);
		m_pinsbyName[pPin->GetName()] = pPin;
	}

	std::shared_ptr<Pin> Package::GetPin(std::string name) const
	{
		auto findIt = m_pinsbyName.find(name);
		if (findIt != m_pinsbyName.end())
		{
			return findIt->second;
		}		
		return nullptr;
	}

	std::shared_ptr<Pin> Package::GetPin(unsigned int index) const
	{
		if (index < m_pins.size())
		{
			return m_pins[index];
		}
		return nullptr;
	}

} // namespace Odb::Lib::ProductModel