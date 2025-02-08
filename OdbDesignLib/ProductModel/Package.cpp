#include "Package.h"
#include <string>
#include "../ProtoBuf/package.pb.h"
#include "Pin.h"
#include <memory>

namespace Odb::Lib::ProductModel
{
	Package::Package(const std::string& name, unsigned int index)
		: m_name(name)
		, m_index(index)
	{
	}

	Package::~Package()
	{
		m_pins.clear();
		m_pinsByName.clear();
	}

	std::string Package::GetName() const
	{
		return m_name;
	}

	const Pin::Vector& Package::GetPins() const
	{
		return m_pins;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Package> Package::to_protobuf() const
	{
		auto pPackageMsg = std::make_unique<Odb::Lib::Protobuf::ProductModel::Package>();
		pPackageMsg->set_name(m_name);
		pPackageMsg->set_index(m_index);
		for (const auto& pPin : m_pins)
		{
			pPackageMsg->add_pins()->CopyFrom(*pPin->to_protobuf());
		}
		for (const auto& kvPin : m_pinsByName)
		{
			(*pPackageMsg->mutable_pinsbyname())[kvPin.first] = *kvPin.second->to_protobuf();
		}
		return pPackageMsg;
	}

	void Package::from_protobuf(const Odb::Lib::Protobuf::ProductModel::Package& message)
	{
		m_name = message.name();
		m_index = message.index();
		for (const auto& pinMsg : message.pins())
		{
			auto pPin = std::make_shared<Pin>("", 0);
			pPin->from_protobuf(pinMsg);
			m_pins.push_back(pPin);
		}
		for (const auto& kvPinMsg : message.pinsbyname())
		{
			auto pPin = std::make_shared<Pin>("", 0);
			pPin->from_protobuf(kvPinMsg.second);
			m_pinsByName[kvPinMsg.first] = pPin;
		}
	}

	unsigned int Package::GetIndex() const
	{
		return m_index;
	}

	void Package::AddPin(const std::string& name)
	{
		auto index = static_cast<unsigned int>(m_pins.size());
		auto pPin = std::make_shared<Pin>(name, index);
		m_pins.push_back(pPin);
		m_pinsByName[pPin->GetName()] = pPin;
	}

	std::shared_ptr<Pin> Package::GetPin(const std::string& name) const
	{
		auto findIt = m_pinsByName.find(name);
		if (findIt != m_pinsByName.end())
		{
			return findIt->second;
		}		
		return nullptr;
	}

	std::shared_ptr<Pin> Package::GetPin(unsigned int index) const
	{
		if (index < m_pins.size())
		{
			return m_pins[index];
		}
		return nullptr;
	}

	const Pin::StringMap& Package::GetPinsByName() const
	{
		return m_pinsByName;
	}

} // namespace Odb::Lib::ProductModel