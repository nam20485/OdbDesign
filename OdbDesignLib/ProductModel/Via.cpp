#include "Via.h"


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

} // namespace Odb::Lib::ProductModel
