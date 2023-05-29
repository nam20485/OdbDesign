#include "Package.h"

namespace Odb::Lib::ProductModel
{

	Package::Package(std::string name)
		: m_name(name)
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

	void Package::AddPin(std::string name, unsigned long index)
	{
		m_pins.push_back(std::make_shared<Pin>(name, index));
	}
}