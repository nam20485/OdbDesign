#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>
#include "../../odbdesign_export.h"
#include "ComponentsFile.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/layerdirectory.pb.h"
#include "FeaturesFile.h"
#include "AttrListFile.h"
#include "../ISaveable.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT LayerDirectory : public IProtoBuffable<Odb::Lib::Protobuf::LayerDirectory>, public ISaveable
	{
	public:
		LayerDirectory(std::filesystem::path path);
		virtual ~LayerDirectory();		

		std::string GetName() const;
		std::filesystem::path GetPath() const;

		bool Parse();
		// Inherited via ISaveable
		bool Save(const std::filesystem::path& directory) override;

		bool ParseComponentsFile(std::filesystem::path directory);
		bool ParseFeaturesFile(std::filesystem::path directory);
		bool ParseAttrListFile(std::filesystem::path directory);

		const ComponentsFile& GetComponentsFile() const;
		const FeaturesFile& GetFeaturesFile() const;
		const AttrListFile& GetAttrListFile() const;

		typedef std::map<std::string, std::shared_ptr<LayerDirectory>> StringMap;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::LayerDirectory> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::LayerDirectory& message) override;

	private:
		std::string m_name;
		std::filesystem::path m_path;

		ComponentsFile m_componentsFile;	
		FeaturesFile m_featuresFile;
		AttrListFile m_attrListFile;

	};
}