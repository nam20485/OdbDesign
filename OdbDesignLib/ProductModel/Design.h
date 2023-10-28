#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <memory>
#include "Net.h"
#include "Component.h"
#include "../FileModel/Design/FileArchive.h"
#include "Via.h"
#include "Package.h"
#include "Part.h"


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Design
	{
	public:	
		Design();
		~Design();

		bool Build(std::string path);
		bool Build(std::shared_ptr<FileModel::Design::FileArchive> pFileModel);	

		std::shared_ptr<FileModel::Design::FileArchive> GetFileModel() const;

		typedef std::vector<std::shared_ptr<Design>> Vector;
		typedef std::map<std::string, std::shared_ptr<Design>> StringMap;

	private:
		std::string m_productModel;
		std::string m_name;

		std::shared_ptr<FileModel::Design::FileArchive> m_pFileModel;

		Net::Vector m_nets;
		Net::StringMap m_netsByName;

		Package::Vector m_packages;
		Package::StringMap m_packagesByName;

		Component::Vector m_components;
		Component::StringMap m_componentsByName;
		
		Part::StringMap m_partsByName;
		
		bool BuildNets();
		bool BuildPackages();
		bool BuildParts();
		bool BuildComponents();
		bool BuildLayerComponents(std::shared_ptr<Odb::Lib::FileModel::Design::ComponentLayerDirectory>& pComponentsLayerDir);
		bool BuildLayerParts(std::shared_ptr<Odb::Lib::FileModel::Design::ComponentLayerDirectory>& pComponentsLayerDir);
		bool BuildPlacements();
		bool BuildLayerPlacements(std::shared_ptr<Odb::Lib::FileModel::Design::ComponentLayerDirectory>& pTopComponentsLayerDir);

	};
}
