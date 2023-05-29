#pragma once

#include "export.h"
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "Pin.h"


namespace Odb::Lib::ProductModel
{
	class DECLSPEC Package
	{
	public:
		Package(std::string name);
		//~Package();

		std::string GetName() const;
		const Pin::Vector& GetPins() const;

		void AddPin(std::string name, unsigned long index);

		typedef std::vector<std::shared_ptr<Package>> Vector;
		typedef std::map<std::string, std::shared_ptr<Package>> StringMap;

	private:
		std::string m_name;
		Pin::Vector m_pins;		

	};
} // namespace Odb::Lib::ProductModel
