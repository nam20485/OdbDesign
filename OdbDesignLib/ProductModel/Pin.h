#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Pin
	{
	public:
		Pin(std::string name, unsigned int index);
		//~Pin();

		std::string GetName() const;
		unsigned int GetIndex() const;

		typedef std::vector<std::shared_ptr<Pin>> Vector;
		typedef std::map<std::string, std::shared_ptr<Pin>> StringMap;

	private:
		std::string m_name;
		unsigned int m_index;

	};
} // namespace Odb::Lib::ProductModel
