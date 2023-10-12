#pragma once

#include "odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Odb::Lib::ProductModel
{
	class Part
	{
	public:
		Part(std::string name);		

		std::string GetName() const;		

		typedef std::vector<std::shared_ptr<Part>> Vector;
		typedef std::map<std::string, std::shared_ptr<Part>> StringMap;

	private:
			std::string m_name;

	};
}
