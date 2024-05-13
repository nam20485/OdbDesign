#include "PinConnection.h"
#include <memory>
#include "Pin.h"
#include "Component.h"
#include <string>
#include "../ProtoBuf/pinconnection.pb.h"


namespace Odb::Lib::ProductModel
{	
	//PinConnection::~PinConnection()
	//{
	//}

	PinConnection::PinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin)
		: PinConnection(pComponent, pPin, MakeName(*pComponent, *pPin))
	{
	}	

	PinConnection::PinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin, const std::string& name)
		: m_name(name)
		, m_pComponent(pComponent)
		, m_pPin(pPin)		
	{
	}

	std::string PinConnection::MakeName(const Component& component, const Pin& pin)
	{		
		//if (pComponent == nullptr || pPin == nullptr) return "Pin::MakeName() FAILED";
		//return pComponent->GetRefDes() + "-" + pPin->GetName();
		return component.GetRefDes() + "-" + pin.GetName();
	}

	std::shared_ptr<Pin> PinConnection::GetPin() const
	{
		return m_pPin;
	}

	std::shared_ptr<Component> PinConnection::GetComponent() const
	{
		return m_pComponent;
	}

	std::unique_ptr<Protobuf::ProductModel::PinConnection> PinConnection::to_protobuf() const
	{
		auto pPinConnectionMsg = std::make_unique<Protobuf::ProductModel::PinConnection>();
		pPinConnectionMsg->set_name(m_name);
		pPinConnectionMsg->mutable_component()->CopyFrom(*m_pComponent->to_protobuf());
		pPinConnectionMsg->mutable_pin()->CopyFrom(*m_pPin->to_protobuf());
		return pPinConnectionMsg;
	}

	void PinConnection::from_protobuf(const Protobuf::ProductModel::PinConnection& message)
	{
		m_name = message.name();
		m_pComponent = std::make_shared<Component>();
		m_pComponent->from_protobuf(message.component());
		m_pPin = std::make_shared<Pin>("", -1);
		m_pPin->from_protobuf(message.pin());
	}

} // namespace Odb::Lib::ProductModel
