#include "Pin.h"


namespace Odb::Lib::ProductModel
{	
	Pin::Pin(std::string name, unsigned int index)
		: m_name(name)
		, m_index(index)
	{
	}

	std::string Pin::GetName() const
	{
		return m_name;
	}

	unsigned int Pin::GetIndex() const
	{
		return m_index;
	}
}
