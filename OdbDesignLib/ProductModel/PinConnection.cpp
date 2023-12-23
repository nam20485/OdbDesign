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

} // namespace Odb::Lib::ProductModel
