#include "Part.h"

namespace Odb::Lib::ProductModel
{
	Part::Part(std::string name)
		: m_name(name)
	{
	}
	std::string Part::GetName() const
	{
		return m_name;
	}
}