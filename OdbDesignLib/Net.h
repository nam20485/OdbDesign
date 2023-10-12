#pragma once

#include "odbdesign_export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "PinConnection.h"
#include "Component.h"
#include "Pin.h"


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Net
	{
	public:
		Net(std::string name, unsigned int index);
		~Net();

		std::string GetName() const;
		const PinConnection::Vector& GetPinConnections() const;
		unsigned int GetIndex() const;
		bool AddPinConnection(std::shared_ptr<Component> pComponent, std::shared_ptr<Pin> pPin, std::string name);

		typedef std::vector<std::shared_ptr<Net>> Vector;
		typedef std::map<std::string, std::shared_ptr<Net>> StringMap;

	private:
		std::string m_name;
		PinConnection::Vector m_pinConnections;
		unsigned int m_index;

	};
}
