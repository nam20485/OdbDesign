#include "Component.h"


namespace Odb::Lib::ProductModel
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
} // namespace Odb::Lib::ProductModel