#include "Net.h"


namespace Odb::Lib::ProductModel
{
	Net::Net(std::string name, unsigned int index)
		: m_name(name)
		, m_index(index)
	{
	}

	Net::~Net()
	{
		m_pinConnections.clear();
	}

	std::string Net::GetName() const
	{
		return m_name;
	}

	PinConnection::Vector& Net::GetPinConnections()
	{
		return m_pinConnections;
	}

	unsigned int Net::GetIndex() const
	{
		return m_index;
	}

	bool Net::AddPinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin)
	{
		m_pinConnections.push_back(std::make_shared<PinConnection>(pComponent, pPin));
		return true;
	}
}
