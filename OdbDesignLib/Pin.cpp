#include "Pin.h"


namespace OdbDesign::Lib::Design
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
