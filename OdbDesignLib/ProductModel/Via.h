#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../enums.h"


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Via
	{
	public:
		Via();

		std::string GetName() const;
		BoardSide GetSide() const;

		typedef std::vector<std::shared_ptr<Via>> Vector;
		typedef std::map<std::string, std::shared_ptr<Via>> StringMap;

	private:
		std::string m_name;
		BoardSide m_side;

	};
} // namespace Odb::Lib::ProductModel
