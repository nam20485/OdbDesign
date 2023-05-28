#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Component.h"
#include "Pin.h"


namespace OdbDesign::Lib::Design
{
	class PinConnection
	{
	public:
		PinConnection();
		~PinConnection();

		//std::string GetPinName() const;
		//std::string GetComponentRefDes() const;

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
