#include "Component.h"


namespace Odb::Lib::ProductModel
{
	Component::Component(std::string refDes, std::string partName, std::shared_ptr<Package> pPackage, unsigned int index, BoardSide side)
		: m_refDes(refDes)
		, m_partName(partName)
		, m_pPackage(pPackage)
		, m_index(index)
		, m_side(side)
	{
	}

	Component::~Component()
	{
	}

	std::string Component::GetRefDes() const
	{
		return m_refDes;
	}

	std::string Component::GetPartName() const
	{
		return m_partName;
	}

	std::shared_ptr<Package> Component::GetPackage() const
	{
		return m_pPackage;
	}

	unsigned int Component::GetIndex() const
	{
		return m_index;
	}

	BoardSide Component::GetSide() const
	{
		return m_side;
	}


} // namespace Odb::Lib::ProductModel