#include "Pin.h"


namespace Odb::Lib::ProductModel
{
	Pin::Pin()
	{
	}

	Pin::~Pin()
	{
	}

	std::string Pin::GetName() const
	{
		return m_name;
	}
}
