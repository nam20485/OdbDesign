#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <memory>
#include "Net.h"
#include "Component.h"
#include "Package.h"
#include "Part.h"
#include "../ProtoBuf/design.pb.h"
#include "../IProtoBuffable.h"	
#include <map>
#include <vector>
#include "../FileModel/Design/StepDirectory.h"
#include "../FileModel/Design/EdaDataFile.h"
#include "../FileModel/Design/ComponentsFile.h"
#include "../FileModel/Design/FileArchive.h"


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Design : public IProtoBuffable<Protobuf::ProductModel::Design>
	{
	public:	
		Design();
		~Design();
		
		const std::string& GetName() const;
		const std::string& GetProductModel() const;
		
		const Net::Vector& GetNets() const;
		const Net::StringMap GetNetsByName() const;
		std::shared_ptr<Net> GetNet(const std::string& name) const;

		const Package::Vector& GetPackages() const;
		const Package::StringMap& GetPackagesByName() const;

		const Component::Vector& GetComponents() const;
		const Component::StringMap& GetComponentsByName() const;
		std::shared_ptr<Component> GetComponent(const std::string& refDes) const;

		const Part::Vector& GetParts() const;
		const Part::StringMap& GetPartsByName() const;
		
		std::shared_ptr<Net> GetNoneNet() const;

		bool Build(std::string path);
		bool Build(std::shared_ptr<FileModel::Design::FileArchive> pFileModel);	

		std::shared_ptr<FileModel::Design::FileArchive> GetFileModel() const;
		void ClipFileModel();

		// Inherited via IProtoBuffable
		std::unique_ptr<Protobuf::ProductModel::Design> to_protobuf() const override;
		void from_protobuf(const Protobuf::ProductModel::Design& message) override;

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
		bool BuildParts(const FileModel::Design::ComponentsFile* pComponentsFile);
		bool BuildAllComponents();
		bool BuildComponents(const FileModel::Design::ComponentsFile* pComponentsFile);	
		bool BuildVias();

		bool BuildPlacementsFromComponentsFiles();
		bool BuildPlacementsFromComponentsFile(const FileModel::Design::ComponentsFile* pComponentsFile);		

		bool BuildPlacementsFromEdaDataFile();
		
		bool BuildNoneNet();
		bool BreakSinglePinNets();

		// helper convienience methods
		bool CreatePinConnection(const std::string& refDes, unsigned int netNumber, unsigned int pinNumber, const std::string& pinName);
		bool CreateNetConnections(const std::shared_ptr<FileModel::Design::EdaDataFile::NetRecord>& pNetRecord, const std::shared_ptr<FileModel::Design::StepDirectory>& pStepDirectory);

		constexpr inline static const char* NONE_NET_NAME = "$NONE$";

		constexpr inline static bool CLIP_FILEMODEL_AFTER_BUILD = false;

	};	
}
