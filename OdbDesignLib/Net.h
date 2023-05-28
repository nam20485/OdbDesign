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
		Net();
		~Net();

		std::string GetName() const;

		typedef std::vector<std::shared_ptr<Net>> Vector;
		typedef std::map<std::string, std::shared_ptr<Net>> StringMap;

	private:
		std::string m_name;
		PinConnection::Vector m_pinConnections;

	};
}
