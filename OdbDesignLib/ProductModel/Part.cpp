#include "Part.h"
#include <string>
#include "../ProtoBuf/part.pb.h"
#include <memory>

namespace Odb::Lib::ProductModel
{
	Part::Part(const std::string& name)
		: m_name(name)
	{
	}

	std::string Part::GetName() const
	{
		return m_name;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Part> Odb::Lib::ProductModel::Part::to_protobuf() const
	{
		auto pPartMsg = std::make_unique<Odb::Lib::Protobuf::ProductModel::Part>();
		pPartMsg->set_name(m_name);
		return pPartMsg;
	}

	void Odb::Lib::ProductModel::Part::from_protobuf(const Odb::Lib::Protobuf::ProductModel::Part& message)
	{
		m_name = message.name();
	}
}