#pragma once

#include "export.h"
#include <string>
#include "Net.h"
#include "Component.h"
#include "FileModel.h"
#include "Via.h"

namespace OdbDesign::Lib::Design
{
	class DECLSPEC Design
	{
	public:
		Design(const OdbDesign::Lib::FileModel::FileModel& fileModel);
		~Design();

	private:
		std::string m_productModel;
		std::string m_name;

		Net::StringMap m_netsByName;
		Component::StringMap m_componentsByName;
		Via::StringMap m_viasByName;

		bool BuildFromFileModel();

	};
}
