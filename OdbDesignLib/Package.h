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
		Package(std::string name, unsigned int index);
		//~Package();

		std::string GetName() const;
		const Pin::Vector& GetPins() const;
		unsigned int GetIndex() const;

		void AddPin(std::string name);

		typedef std::vector<std::shared_ptr<Package>> Vector;
		typedef std::map<std::string, std::shared_ptr<Package>> StringMap;

	private:
		std::string m_name;
		Pin::Vector m_pins;		
		unsigned int m_index;

	};
} // namespace Odb::Lib::ProductModel
