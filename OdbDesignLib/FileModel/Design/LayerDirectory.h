#pragma once

#include <string>
#include <map>
#include <memory>
#include <filesystem>
#include "../../odbdesign_export.h"
#include "ComponentsFile.h"
#include "../../IProtoBuffable.h"
#include "../../ProtoBuf/layerdirectory.pb.h"


namespace Odb::Lib::FileModel::Design
{
	class ODBDESIGN_EXPORT LayerDirectory : public IProtoBuffable<Odb::Lib::Protobuf::LayerDirectory>
	{
	public:
		LayerDirectory(std::filesystem::path path);
		virtual ~LayerDirectory();		

		std::string GetName() const;
		std::filesystem::path GetPath() const;

		virtual bool Parse();

		bool ParseComponentsFile(std::filesystem::path directory);
		bool ParseFeaturesFile(std::filesystem::path directory);

		const ComponentsFile& GetComponentsFile() const;

		typedef std::map<std::string, std::shared_ptr<LayerDirectory>> StringMap;

		// Inherited via IProtoBuffable
		std::unique_ptr<Odb::Lib::Protobuf::LayerDirectory> to_protobuf() const override;
		void from_protobuf(const Odb::Lib::Protobuf::LayerDirectory& message) override;

	protected: // TODO: do subclasses really need access to these (private instead)?
		std::string m_name;
		std::filesystem::path m_path;

	private:
		ComponentsFile m_componentsFile;		

	};
}