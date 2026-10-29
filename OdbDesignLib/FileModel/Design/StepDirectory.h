#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>

#include "../../odbdesign_export.h"
#include "LayerDirectory.h"
#include "EdaDataFile.h"
#include "NetlistFile.h"
#include "../../IProtoBuffable.h"
#include "stepdirectory.pb.h"
#include "ComponentsFile.h"
#include "AttrListFile.h"
#include "StepHdrFile.h"
#include "../ISaveable.h"
#include "FeaturesFile.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT StepDirectory : public IProtoBuffable<Odb::Lib::Protobuf::StepDirectory>, public ISaveable
	{
	public:
		StepDirectory(std::filesystem::path path);
		~StepDirectory();

		std::string GetName();
		std::filesystem::path GetPath();

		const EdaDataFile& GetEdaDataFile() const;
		const LayerDirectory::StringMap& GetLayersByName() const;
		const NetlistFile::StringMap& GetNetlistsByName() const;
		const AttrListFile& GetAttrListFile() const;
		const FeaturesFile& GetProfileFile() const;
		const StepHdrFile& GetStepHdrFile() const;

		const ComponentsFile* GetTopComponentsFile() const;
		const ComponentsFile* GetBottomComponentsFile() const;

		bool Parse();
		// Inherited via ISaveable
		bool Save(const std::filesystem::path& directory) override;

		typedef std::map<std::string, std::shared_ptr<StepDirectory>> StringMap;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::StepDirectory> to_protobuf() const override;
		// M6 streaming: same message with every layer's feature records
		// omitted (layers serialized as skeletons; the records are delivered
		// via FeatureBatch chunks). Profile/eda/netlist/components ride whole.
		std::unique_ptr<Odb::Lib::Protobuf::StepDirectory> to_protobuf_skeleton() const;
		void from_protobuf(const Odb::Lib::Protobuf::StepDirectory& message) override;

	private:
		std::string m_name;
		std::filesystem::path m_path;

		LayerDirectory::StringMap m_layersByName;
		NetlistFile::StringMap m_netlistsByName;
		EdaDataFile m_edaData;
		AttrListFile m_attrListFile;
		FeaturesFile m_profileFile;
		StepHdrFile m_stepHdrFile;

		bool ParseLayerFiles(std::filesystem::path layersPath);
		bool ParseNetlistFiles(std::filesystem::path netlistsPath);
		bool ParseEdaDataFiles(std::filesystem::path edaPath);
		bool ParseAttrListFile(std::filesystem::path attrListFileDirectory);
		bool ParseProfileFile(std::filesystem::path profileFileDirectory);
		bool ParseStepHdrFile(std::filesystem::path stepHdrFileDirectory);

		// Shared body of to_protobuf() / to_protobuf_skeleton(); only the
		// layer serialization differs, keeping the skeleton byte-compatible
		// with the full serialization.
		void FillProtobufMessage(Odb::Lib::Protobuf::StepDirectory& stepDirectoryMessage, bool layerSkeletons) const;


		constexpr inline static const char* PROFILE_FILENAME = "profile";
		
	};
}


