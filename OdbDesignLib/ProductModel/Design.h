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
//#include "../FileModel/Design/StepDirectory.h"
#include "../ProtoBuf/design.pb.h"
#include "../IProtoBuffable.h"	


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Design : IProtoBuffable<Odb::Lib::Protobuf::ProductModel::Design>
	{
	public:	
		Design();
		~Design();
		
		const std::string& GetName() const;
		const std::string& GetProductModel() const;
		
		const Net::Vector& GetNets() const;
		const Net::StringMap GetNetsByName() const;

		const Package::Vector& GetPackages() const;
		const Package::StringMap& GetPackagesByName() const;

		const Component::Vector& GetComponents() const;
		const Component::StringMap& GetComponentsByName() const;

		const Part::Vector& GetParts() const;
		const Part::StringMap& GetPartsByName() const;

		std::shared_ptr<Net> GetNet(const std::string& name) const;
		std::shared_ptr<Net> GetNoneNet() const;

		bool Build(std::string path);
		bool Build(std::shared_ptr<FileModel::Design::FileArchive> pFileModel);	

		std::shared_ptr<FileModel::Design::FileArchive> GetFileModel() const;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::ProductModel::Design> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::ProductModel::Design& message) override;

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
		
		Part::Vector m_parts;
		Part::StringMap m_partsByName;

		bool BuildNets();
		bool BuildPackages();
		bool BuildAllParts();
		bool BuildParts(const Odb::Lib::FileModel::Design::ComponentsFile* pComponentsFile);
		bool BuildAllComponents();
		bool BuildComponents(const Odb::Lib::FileModel::Design::ComponentsFile* pComponentsFile);	
		bool BuildVias();

		bool BuildPlacementsFromComponentsFiles();
		bool BuildPlacementsFromComponentsFile(const Odb::Lib::FileModel::Design::ComponentsFile* pComponentsFile);		

		bool BuildPlacementsFromEdaDataFile();
		
		bool BuildNoneNet();
		bool BreakSinglePinNets();

		// helper convienience methods
		bool CreatePinConnection(const std::string& refDes, unsigned int netNumber, unsigned int pinNumber, const std::string& pinName);
		bool CreateNetConnections(const std::shared_ptr<FileModel::Design::EdaDataFile::NetRecord>& pNetRecord, const std::shared_ptr<FileModel::Design::StepDirectory>& pStepDirectory);

		constexpr inline static const char* NONE_NET_NAME = "$NONE$";

	};	
}
