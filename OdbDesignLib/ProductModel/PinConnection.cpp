#include "PinConnection.h"
#include "PinConnection.h"
#include "PinConnection.h"
#include "PinConnection.h"
#include "PinConnection.h"


namespace Odb::Lib::ProductModel
{	
	//PinConnection::~PinConnection()
	//{
	//}

	PinConnection::PinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin)
		: PinConnection(pComponent, pPin, MakeName(pComponent, pPin))
	{
	}	

	PinConnection::PinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin, std::string name)
		: m_name(name)
		, m_pComponent(pComponent)
		, m_pPin(pPin)		
	{
	}

	std::string PinConnection::MakeName(std::shared_ptr<Odb::Lib::ProductModel::Component>& pComponent, std::shared_ptr<Odb::Lib::ProductModel::Pin>& pPin)
	{
		return pComponent->GetRefDes() + "-" + pPin->GetName();
	}

	std::shared_ptr<Pin> PinConnection::GetPin() const
	{
		return m_pPin;
	}

	std::shared_ptr<Component> PinConnection::GetComponent() const
	{
		return m_pComponent;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ProductModel::PinConnection> PinConnection::to_protobuf() const
	{
		auto pPinConnectionMsg = std::make_unique<Odb::Lib::Protobuf::ProductModel::PinConnection>();
		pPinConnectionMsg->set_name(m_name);
		pPinConnectionMsg->set_allocated_component(m_pComponent->to_protobuf().release());
		pPinConnectionMsg->set_allocated_pin(m_pPin->to_protobuf().release());
		return pPinConnectionMsg;
	}

	void PinConnection::from_protobuf(const Odb::Lib::Protobuf::ProductModel::PinConnection& message)
	{
		m_name = message.name();
		m_pComponent->from_protobuf(message.component());
		m_pPin->from_protobuf(message.pin());
	}

} // namespace Odb::Lib::ProductModel
