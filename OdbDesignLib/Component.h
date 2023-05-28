#pragma once

#include "export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Pin.h"

namespace Odb::Lib::ProductModel
{
	class DECLSPEC Component
	{
	public:
		Component();
		~Component();

		std::string GetRefDes() const;

		typedef std::vector<std::shared_ptr<Component>> Vector;
		typedef std::map<std::string, std::shared_ptr<Component>> StringMap;

	private:
		std::string m_refDes;
		Pin::Vector m_pins;

	};
}
