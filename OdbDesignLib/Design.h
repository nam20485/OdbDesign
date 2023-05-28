#pragma once

#include "export.h"
#include <string>
#include <memory>
#include "Net.h"
#include "Component.h"
#include "FileModel.h"
#include "Via.h"

namespace OdbDesign::Lib::Design
{
	class DECLSPEC Design
	{
	public:	
		Design();
		~Design();

		bool Build(std::string designDirectory);
		bool Build(std::shared_ptr<FileModel::FileModel> pFileModel);

	private:
		std::string m_productModel;
		std::string m_name;

		std::string m_designDirectory;		
		std::shared_ptr<FileModel::FileModel> m_pFileModel;		

		Net::StringMap m_netsByName;
		Component::StringMap m_componentsByName;
		Via::StringMap m_viasByName;

		bool Build();
		bool BuildComponents();
		bool BuildNets();

	};
}
