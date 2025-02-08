#include "Via.h"
#include <memory>
#include "../ProtoBuf/via.pb.h"
#include "../ProtoBuf/enums.pb.h"
#include "../enums.h"
#include <string>


namespace Odb::Lib::ProductModel
{
	Via::Via()
		: m_name("")
		, m_side(BoardSide::Top)
	{
	}

	std::string Via::GetName() const
	{
		return m_name;
	}

	BoardSide Via::GetSide() const
	{
		return m_side;
	}

	std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Via> Via::to_protobuf() const
	{
		auto pViaMsg = std::make_unique<Odb::Lib::Protobuf::ProductModel::Via>();
		pViaMsg->set_name(m_name);
		pViaMsg->set_boardside(static_cast<Odb::Lib::Protobuf::BoardSide>(m_side));
		return pViaMsg;
	}

	void Via::from_protobuf(const Odb::Lib::Protobuf::ProductModel::Via& message)
	{
		m_name = message.name();
		m_side = static_cast<BoardSide>(message.boardside());
	}

} // namespace Odb::Lib::ProductModel
