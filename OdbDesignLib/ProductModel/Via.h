#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Via
	{
	public:
		Via();
		~Via();

		std::string GetName() const;

		typedef std::vector<std::shared_ptr<Via>> Vector;
		typedef std::map<std::string, std::shared_ptr<Via>> StringMap;

	private:
		std::string m_name;

	};
} // namespace Odb::Lib::ProductModel
