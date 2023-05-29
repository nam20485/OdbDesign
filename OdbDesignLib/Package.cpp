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

	const Pin::Vector& Package::GetPins() const
	{
		return m_pins;
	}

	unsigned int Package::GetIndex() const
	{
		return m_index;
	}

	void Package::AddPin(std::string name)
	{
		auto index = static_cast<unsigned int>(m_pins.size());
		m_pins.push_back(std::make_shared<Pin>(name, index));
	}

} // namespace Odb::Lib::ProductModel