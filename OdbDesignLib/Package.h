#pragma once

#include "odbdesign_export.h"
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "Pin.h"


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Package
	{
	public:
		Package(std::string name, unsigned int index);
		//~Package();

		std::string GetName() const;		
		unsigned int GetIndex() const;

		void AddPin(std::string name);		
		std::shared_ptr<Pin> GetPin(std::string name) const;
		std::shared_ptr<Pin> GetPin(unsigned int index) const;
		//const Pin::StringMap& GetPinsByName() const;		
		//const Pin::Vector& GetPins() const;

		typedef std::vector<std::shared_ptr<Package>> Vector;
		typedef std::map<std::string, std::shared_ptr<Package>> StringMap;

	private:
		std::string m_name;
		Pin::Vector m_pins;		
		Pin::StringMap m_pinsbyName;
		unsigned int m_index;

	};
} // namespace Odb::Lib::ProductModel
