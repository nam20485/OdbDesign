#include "Net.h"
#include <memory>
#include "Component.h"
#include "Pin.h"
#include "../ProtoBuf/net.pb.h"
#include <string>
#include "PinConnection.h"


namespace Odb::Lib::ProductModel
{
	Net::Net(const std::string& name, unsigned int index)
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

	std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Net> Odb::Lib::ProductModel::Net::to_protobuf() const
	{
		auto pNetMsg = std::make_unique<Odb::Lib::Protobuf::ProductModel::Net>();
		pNetMsg->set_name(m_name);
		pNetMsg->set_index(m_index);
		for (auto& pPinConnection : m_pinConnections)
		{
			pNetMsg->add_pinconnections()->CopyFrom(*pPinConnection->to_protobuf());
		}
		return pNetMsg;
	}

	void Odb::Lib::ProductModel::Net::from_protobuf(const Odb::Lib::Protobuf::ProductModel::Net& message)
	{
		m_name = message.name();
		m_index = message.index();
		for (auto& pinConnectionMsg : message.pinconnections())
		{
			auto pPinConnection = std::make_shared<PinConnection>(nullptr, nullptr, "");
			pPinConnection->from_protobuf(pinConnectionMsg);
			m_pinConnections.push_back(pPinConnection);
		}
	}
}
