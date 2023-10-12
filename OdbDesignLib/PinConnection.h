#pragma once

#include "odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Component.h"
#include "Pin.h"


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT PinConnection
	{
	public:
		PinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin, std::string name);
		//~PinConnection();	

		std::shared_ptr<Pin> GetPin() const;
		std::shared_ptr<Component> GetComponent() const;

		typedef std::vector<std::shared_ptr<PinConnection>> Vector;
		typedef std::map<std::string, std::shared_ptr<PinConnection>> StringMap;

	private:
		std::string m_name;
		std::shared_ptr<Component> m_pComponent;
		std::shared_ptr<Pin> m_pPin;

	};
}
