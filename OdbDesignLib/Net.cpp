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
	}

	std::string Net::GetName() const
	{
		return m_name;
	}
	const PinConnection::Vector& Net::GetPinConnections() const
	{
		return m_pinConnections;
	}
	unsigned int Net::GetIndex() const
	{
		return m_index;
	}
}
