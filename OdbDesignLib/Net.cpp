#include "Net.h"


namespace Odb::Lib::ProductModel
{
	Net::Net(std::string name)
		: m_name(name)
	{
	}

	Net::~Net()
	{
	}

	std::string Net::GetName() const
	{
		return m_name;
	}
}
