#include "Component.h"


namespace OdbDesign::Lib::Design
{
	Component::Component()
	{
	}

	Component::~Component()
	{
	}

	std::string Component::GetRefDes() const
	{
		return m_refDes;
	}
} // namespace OdbDesign::Lib::Design