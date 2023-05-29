#include "PinConnection.h"


namespace Odb::Lib::ProductModel
{
	PinConnection::PinConnection()
	{
	}

	PinConnection::~PinConnection()
	{
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
