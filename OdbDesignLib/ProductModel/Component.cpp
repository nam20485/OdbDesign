#include "Component.h"
#include "Component.h"


namespace Odb::Lib::ProductModel
{
	Component::Component(std::string refDes, std::string partName, std::shared_ptr<Package> pPackage, unsigned int index, BoardSide side, std::shared_ptr<Part> pPart)
		: m_refDes(refDes)
		, m_partName(partName)
		, m_pPackage(pPackage)
		, m_index(index)
		, m_side(side)
		, m_pPart(pPart)
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

	std::shared_ptr<Part> Component::GetPart() const
	{
		return m_pPart;
	}

} // namespace Odb::Lib::ProductModel