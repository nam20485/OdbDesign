#pragma once

#include "export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "PinConnection.h"


namespace Odb::Lib::ProductModel
{
	class DECLSPEC Net
	{
	public:
		Net(std::string name, unsigned int index);
		~Net();

		std::string GetName() const;
		const PinConnection::Vector& GetPinConnections() const;
		unsigned int GetIndex() const;

		typedef std::vector<std::shared_ptr<Net>> Vector;
		typedef std::map<std::string, std::shared_ptr<Net>> StringMap;

	private:
		std::string m_name;
		PinConnection::Vector m_pinConnections;
		unsigned int m_index;

	};
}
