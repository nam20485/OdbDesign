#include "Component.h"
#include "Component.h"
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

	std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Component> Component::to_protobuf() const
	{
		auto pComponentMsg = std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Component>();
		pComponentMsg->set_refdes(m_refDes);
		pComponentMsg->set_partname(m_partName);
		pComponentMsg->set_index(m_index);
		pComponentMsg->set_side(static_cast<Odb::Lib::Protobuf::BoardSide>(m_side));
		pComponentMsg->set_allocated_package(m_pPackage->to_protobuf().release());
		pComponentMsg->set_allocated_part(m_pPart->to_protobuf().release());
		return pComponentMsg;
	}

	void Component::from_protobuf(const Odb::Lib::Protobuf::ProductModel::Component& message)
	{
		m_refDes = message.refdes();
		m_partName = message.partname();
		m_index = message.index();
		m_side = static_cast<BoardSide>(message.side());
		m_pPackage->from_protobuf(message.package());
		m_pPart->from_protobuf(message.part());
	}

} // namespace Odb::Lib::ProductModel