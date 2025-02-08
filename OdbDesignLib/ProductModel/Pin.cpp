#include "Pin.h"
#include <string>
#include <memory>
#include "../ProtoBuf/pin.pb.h"

namespace Odb::Lib::ProductModel
{	
	Pin::Pin(const std::string& name, unsigned int index)
		: m_name(name)
		, m_index(index)
	{
	}

	std::string Pin::GetName() const
	{
		return m_name;
	}

	unsigned int Pin::GetIndex() const
	{
		return m_index;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Pin> Odb::Lib::ProductModel::Pin::to_protobuf() const
	{
		auto pPinMessage =  std::make_unique<Odb::Lib::Protobuf::ProductModel::Pin>();
		pPinMessage->set_name(m_name);
		pPinMessage->set_index(m_index);
		return pPinMessage;
	}

	void Pin::from_protobuf(const Odb::Lib::Protobuf::ProductModel::Pin& message)
	{
		m_name = message.name();
		m_index = message.index();
	}	
}
