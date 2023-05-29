#pragma once

#include "export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>


namespace Odb::Lib::ProductModel
{
	class DECLSPEC Via
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
